/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Authentication.h"
#include "ClientContext.h"
#include "../AccountClient.h"
#include "../ClientConnection.h"
#include "../ClientHandler.h"
#include "../ClientLogHelper.h"
#include "../ConfigStore.h"
#include "../Digest.h"
#include "../EventDispatcher.h"
#include "../Events.h"
#include "../RealmQueue.h"
#include <logger/Logger.h>
#include <protocol/Opcodes.h>
#include <protocol/PacketHeaders.h>
#include <protocol/Packets.h>
#include <spark/buffers/pmr/Buffer.h>
#include <shared/game/GameVersion.h>
#include <shared/utility/EnumHelper.h>
#include <shared/utility/UTF8String.h>
#include <shared/utility/xoroshiro128plus.h>
#include <boost/container/small_vector.hpp>
#include <gsl/narrow>
#include <algorithm>
#include <utility>
#include <cstddef>
#include <cstdint>

namespace ember::realm::authentication {

using AddonData = protocol::client::AuthSession::AddonData;

void send_auth_challenge(ClientContext& ctx);
void send_auth_result(ClientContext& ctx, protocol::Result result);
void handle_authentication(ClientContext& ctx);
void handle_queue_update(ClientContext& ctx, const QueuePosition* event);
void handle_queue_success(ClientContext& ctx);
void auth_success(ClientContext& ctx);
void auth_queue(ClientContext& ctx);
void prove_session(ClientContext& ctx, const Botan::BigInt& key);
void fetch_session_key(const ClientContext& ctx, std::uint32_t account_id);
void fetch_account_id(const ClientContext& ctx, const utf8_string& username);
void handle_timeout(ClientContext& ctx);
void send_addon_data(ClientContext& ctx);

void auth_state(ClientContext& ctx, State state) {
	auto& state_ctx = std::get<Context>(ctx.state_ctx);
	state_ctx.state = state;
}

State auth_state(ClientContext& ctx) {
	const auto& state_ctx = std::get<Context>(ctx.state_ctx);
	return state_ctx.state;
}

void handle_authentication(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	// prevent repeated auth attempts
	if(auth_state(ctx) != State::not_authed) {
		return;
	}

	auto& auth_ctx = std::get<Context>(ctx.state_ctx);

	if(!ctx.handler.deserialise(auth_ctx.packet, *ctx.stream)) {
		return;
	}

	CLIENT_DEBUG(ctx.logger, ctx)
		<< "Received session proof for "
		<< auth_ctx.packet->username
		<< LOG_ASYNC;
	
	const auto& config = ctx.cfg_store.config_tls();

	const bool build_res = std::ranges::any_of(config.allowed_builds, [&](auto& version) {
		return version.build == auth_ctx.packet->build;
	});

	if(!build_res) {
		CLIENT_DEBUG(ctx.logger, ctx)
			<< "Build validation failed for "
			<< auth_ctx.packet->username
			<< LOG_ASYNC;

		auth_state(ctx, State::failed);
		ctx.handler.close();
		return;
	}

	auth_state(ctx, State::in_progress);
	fetch_account_id(ctx, auth_ctx.packet->username);
}

void fetch_account_id(const ClientContext& ctx, const utf8_string& username) {
	LOG_TRACE(ctx.logger, log_func);

	const auto& uuid = ctx.handler.uuid();
	auto& dispatcher = ctx.dispatcher;

	ctx.account_rpc.locate_account_id(username, [dispatcher, uuid](auto status, auto id) {
		AccountIDResponse event(status, id);
		dispatcher.post_event(uuid, event);
	});
}

void handle_account_id(ClientContext& ctx, const AccountIDResponse* event) {
	LOG_TRACE(ctx.logger, log_func);
	
	auto& auth_ctx = std::get<Context>(ctx.state_ctx);

	if(event->status != rpc::Account::Status::ok) {
		CLIENT_ERROR(ctx.logger, ctx)
			<< "Account server returned "
			<< utility::fb_status(event->status, rpc::Account::EnumNamesStatus())
			<< " for " << auth_ctx.packet->username << " lookup" << LOG_ASYNC;

		auth_state(ctx, State::failed);
		ctx.handler.close();
		return;
	}

	if(event->account_id) {
		auth_ctx.account_id = event->account_id;
		fetch_session_key(ctx, event->account_id);
	} else {
		CLIENT_DEBUG(ctx.logger, ctx)
			<< "Account ID lookup for failed for "
			<< auth_ctx.packet->username << LOG_ASYNC;
		auth_state(ctx, State::failed);
		ctx.handler.close();
	}
}

void fetch_session_key(const ClientContext& ctx, const std::uint32_t account_id) {
	LOG_TRACE(ctx.logger, log_func);

	const auto& uuid = ctx.handler.uuid();
	auto& dispatcher = ctx.dispatcher;
		
	ctx.account_rpc.locate_session(account_id, [dispatcher, uuid](auto status, auto key) {
		SessionKeyResponse event(status, key);
		dispatcher.post_event(uuid, event);
	});
}

void handle_session_key(ClientContext& ctx, const SessionKeyResponse* event) {
	const auto& auth_ctx = std::get<Context>(ctx.state_ctx);

	CLIENT_DEBUG(ctx.logger, ctx)
		<< "Account server returned "
		<< utility::fb_status(event->status, rpc::Account::EnumNamesStatus())
		<< " for " << auth_ctx.packet->username << LOG_ASYNC;

	if(event->status == rpc::Account::Status::ok) {
		prove_session(ctx, event->key);
	} else {
		auth_state(ctx, State::failed);
		ctx.handler.close();
	}
}

void prove_session(ClientContext& ctx, const Botan::BigInt& key) {
	LOG_TRACE(ctx.logger, log_func);

	// Encode the key without requiring an allocation
	static constexpr auto key_size_hint = 40u;

	boost::container::small_vector<std::uint8_t, key_size_hint> k_bytes(
		key.bytes(), boost::container::default_init
	);

	key.serialize_to(k_bytes);

	// hardcoded to zero in the client, either proto ID or challenge version
	const std::uint32_t protocol_id = 0;
	auto& auth_ctx = std::get<Context>(ctx.state_ctx);

	const digest::Context params {
		.key = k_bytes,
		.username = auth_ctx.packet->username,
		.protocol_id = protocol_id,
		.client_seed = auth_ctx.packet->seed,
		.server_seed = auth_ctx.seed,
	};

	if(!digest::validate(params, auth_ctx.packet->digest)) {
		CLIENT_DEBUG(ctx.logger, ctx) << "Received bad digest for " << auth_ctx.packet->username << LOG_ASYNC;
		auth_state(ctx, State::failed);
		ctx.handler.close(); // key mismatch, client can't decrypt response
		return;
	}

	ctx.connection->set_key(k_bytes);
	ctx.client_id = { auth_ctx.account_id, auth_ctx.packet->username };

	 // todo, allowing for multiple realms to connect to a single world server
	 // will require an external service to keep track of available slots
	unsigned int active_players = 0;
	const auto& config = ctx.cfg_store.config_tls();

	if(active_players < config.max_slots) {
		auth_success(ctx);
	} else {
		auth_queue(ctx);
	}
}

void send_auth_challenge(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	auto& auth_ctx = std::get<Context>(ctx.state_ctx);
	protocol::smsg_auth_challenge response;
	response->seed = auth_ctx.seed = gsl::narrow_cast<std::uint32_t>(rng::xorshift::next());
	ctx.connection->send(response);
}

void send_addon_data(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	const auto& auth_ctx = std::get<Context>(ctx.state_ctx);
	const auto& addons = auth_ctx.packet->addons;
	protocol::smsg_addon_info response;

	// todo, use AddonData.dbc
	for(const auto& addon : addons) {
		CLIENT_DEBUG(ctx.logger, ctx) << "Addon: " << addon.name << ", Key version: " << addon.key_version
			<< ", CRC: " << addon.crc << ", URL CRC: " << addon.update_url_crc << LOG_ASYNC;

		protocol::server::AddonInfo::AddonData data;
		data.type = protocol::server::AddonInfo::AddonData::Type::blizzard;
		data.update_available_flag = 0; // URL must be present for this to work (check URL CRC)

		if(addon.key_version != 0 && addon.crc != 0x4C1C776D) { // todo, define?
			CLIENT_DEBUG(ctx.logger, ctx) << "Repairing " << addon.name << LOG_ASYNC;
			data.key_version = 1;
		} else {
			data.key_version = 0;
		}		
		
		response->addon_data.emplace_back(std::move(data));
	}

	ctx.connection->send(response);
}

void auth_queue(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	const auto& uuid = ctx.handler.uuid();
	auto& dispatcher = ctx.dispatcher;

	ctx.queue.enqueue(uuid,
		[dispatcher, uuid](const std::size_t position) {
			dispatcher.post_event(uuid, QueuePosition(position));
		},
		[dispatcher, uuid]() {
			const Event event { EventType::queue_success };
			dispatcher.post_event(uuid, event);
		}
	);

	ctx.handler.cancel_timer();
	auth_state(ctx, State::in_queue);
	CLIENT_DEBUG(ctx.logger, ctx) << "added to queue" << LOG_ASYNC;
}

void auth_success(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	send_auth_result(ctx, protocol::Result::auth_ok);
	send_addon_data(ctx);
	auth_state(ctx, State::success);
	ctx.handler.state_update(ClientState::cs_character_list);
	CLIENT_DEBUG(ctx.logger, ctx) << "authenticated" << LOG_ASYNC;
}

void send_auth_result(ClientContext& ctx, protocol::Result result) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::smsg_auth_response response;
	response->result = result;
	ctx.connection->send(response);
}

void handle_queue_update(ClientContext& ctx, const QueuePosition* event) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::smsg_auth_response packet;
	packet->result = protocol::Result::auth_wait_queue;
	packet->queue_position = gsl::narrow_cast<std::uint32_t>(event->position);
	ctx.connection->send(packet);
}

void handle_queue_success(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);
	auth_success(ctx);
}

void handle_timeout(ClientContext& ctx) {
	CLIENT_DEBUG(ctx.logger, ctx) << "Authentication timed out" << LOG_ASYNC;
	ctx.handler.close();
}

void enter(ClientContext& ctx) {
	ctx.state_ctx = Context{};
	const auto& config = ctx.cfg_store.config_tls();
	ctx.handler.start_timer(config.auth_timeout);
	send_auth_challenge(ctx);
}

void handle_packet(ClientContext& ctx, protocol::ClientOpcode opcode) {
	switch(opcode) {
		case protocol::ClientOpcode::cmsg_auth_session:
			handle_authentication(ctx);
			break;
		default:
			ctx.handler.skip(*ctx.stream);
	}
}

void handle_event(ClientContext& ctx, const Event* event) {
	switch(event->type) {
		case EventType::timer_expired:
			handle_timeout(ctx);
			break;
		case EventType::account_id_response:
			handle_account_id(ctx, static_cast<const AccountIDResponse*>(event));
			break;
		case EventType::session_key_response:
			handle_session_key(ctx, static_cast<const SessionKeyResponse*>(event));
			break;
		case EventType::queue_update_position:
			handle_queue_update(ctx, static_cast<const QueuePosition*>(event));
			break;
		case EventType::queue_success:
			handle_queue_success(ctx);
			break;
		default:
			break;
	}
}

void exit(ClientContext& ctx) {
	const auto& auth_ctx = std::get<Context>(ctx.state_ctx);

	if(auth_ctx.state == State::in_queue) {
		ctx.queue.dequeue(ctx.handler.uuid());
	} else {
		ctx.handler.cancel_timer();
	}
}

} // authentication, realm, ember
