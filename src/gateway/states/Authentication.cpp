/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Authentication.h"
#include "../ClientConnection.h"
#include <game_protocol/Opcodes.h>
#include <game_protocol/PacketHeaders.h>
#include <game_protocol/Packets.h>
#include <spark/Buffer.h>
#include <spark/temp/Account_generated.h>
#include <shared/util/xoroshiro128plus.h>
#include <logger/Logging.h>
#include <botan/botan.h>
#include <botan/sha160.h>
#include <cstdint>

#include "../temp.h"

#undef ERROR // temp

namespace em = ember::messaging;

namespace ember {

namespace {

void send_auth_challenge(ClientContext* ctx);
void send_auth_result(ClientContext* ctx, protocol::ResultCode result);
void handle_authentication(ClientContext* ctx);
void prove_session(ClientContext* ctx, Botan::BigInt key, const protocol::CMSG_AUTH_SESSION& packet);
void fetch_session_key(ClientContext* ctx, const protocol::CMSG_AUTH_SESSION& packet);
void fetch_account_id(ClientContext* ctx, const protocol::CMSG_AUTH_SESSION& packet);

void handle_authentication(ClientContext* ctx) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	// prevent repeated auth attempts
	if(ctx->auth_status != AuthStatus::NOT_AUTHED) {
		return;
	}
	
	ctx->auth_status = AuthStatus::IN_PROGRESS;

	protocol::CMSG_AUTH_SESSION packet;
	packet.set_size(ctx->header->size - sizeof(protocol::ClientHeader::opcode));

	if(!ctx->handler->packet_deserialise(packet, *ctx->buffer)) {
		return;
	}

	LOG_DEBUG_GLOB << "Received session proof from " << packet.username << LOG_ASYNC;

	// todo - check game build
	fetch_account_id(ctx, packet);
}

void fetch_account_id(ClientContext* ctx, const protocol::CMSG_AUTH_SESSION& packet) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	auto self = ctx->connection->shared_from_this();

	acct_serv->locate_account_id(packet.username, [self, ctx, packet](em::account::Status remote_res,
	                                                                  std::uint32_t account_id) {
		ctx->connection->socket().get_io_service().dispatch([self, ctx, packet, remote_res, account_id]() { // todo
			if(remote_res == em::account::Status::OK && account_id) {
				if((ctx->account_id = account_id)) {
					fetch_session_key(ctx, packet);
				} else {
					LOG_DEBUG_FILTER_GLOB(LF_NETWORK) << "Account ID lookup for failed for "
						<< packet.username << LOG_ASYNC;
				}
			} else {
				LOG_ERROR_FILTER_GLOB(LF_NETWORK)
					<< "Account server returned " << em::account::EnumNameStatus(remote_res)
					<< " for " << packet.username << " lookup" << LOG_ASYNC;
				ctx->connection->close_session();
			}
		}
	);});
}

void fetch_session_key(ClientContext* ctx, const protocol::CMSG_AUTH_SESSION& packet) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	auto self = ctx->connection->shared_from_this();

	acct_serv->locate_session(ctx->account_id, [self, ctx, packet](em::account::Status remote_res,
	                                                               Botan::BigInt key) {
		ctx->connection->socket().get_io_service().dispatch([self, ctx, packet, remote_res, key]() { // todo
			LOG_DEBUG_FILTER_GLOB(LF_NETWORK)
				<< "Account server returned " << em::account::EnumNameStatus(remote_res)
				<< " for " << packet.username << LOG_ASYNC;
	
			ctx->auth_status = AuthStatus::FAILED; // default unless overridden by success

			if(remote_res != em::account::Status::OK) {
				protocol::ResultCode result;

				switch(remote_res) {
					case em::account::Status::ALREADY_LOGGED_IN:
						result = protocol::ResultCode::AUTH_ALREADY_ONLINE;
						break;
					case em::account::Status::SESSION_NOT_FOUND:
						result = protocol::ResultCode::AUTH_UNKNOWN_ACCOUNT;
						break;
					default:
						LOG_ERROR_FILTER_GLOB(LF_NETWORK) << "Received "
							<< em::account::EnumNameStatus(remote_res)
							<< " from account server" << LOG_ASYNC;
						result = protocol::ResultCode::AUTH_SYSTEM_ERROR;
				}

				// note: the game doesn't seem to pay attention to this
				send_auth_result(ctx, result);
			} else {
				prove_session(ctx, key, packet);
			}
		});
	});
}

void send_auth_challenge(ClientContext* ctx) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;
	protocol::SMSG_AUTH_CHALLENGE response;
	response.seed = ctx->auth_seed = static_cast<std::uint32_t>(ember::rng::xorshift::next());
	ctx->connection->send(response);
}

void send_addon_data(ClientContext* ctx, const protocol::CMSG_AUTH_SESSION& packet) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	protocol::SMSG_ADDON_INFO response;

	// todo, use AddonData.dbc
	for(auto& addon : packet.addons) {
		LOG_DEBUG_GLOB << "Addon: " << addon.name << ", Key version: " << addon.key_version
			<< ", CRC: " << addon.crc << ", URL CRC: " << addon.update_url_crc << LOG_ASYNC;

		protocol::SMSG_ADDON_INFO::AddonData data;
		data.type = protocol::SMSG_ADDON_INFO::AddonData::Type::BLIZZARD;
		data.update_available_flag = false; // URL must be present for this to work (check URL CRC)

		if(addon.crc != 0x4C1C776D || addon.key_version != 1) { // todo, define?
			LOG_DEBUG_GLOB << "Repairing " << addon.name << "..." << LOG_ASYNC;
			data.key_version = 1;
		} else {
			data.key_version = 0;
		}		
		
		response.addon_data.emplace_back(std::move(data));
	}

	ctx->connection->send(response);
}

void prove_session(ClientContext* ctx, Botan::BigInt key, const protocol::CMSG_AUTH_SESSION& packet) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	Botan::SecureVector<Botan::byte> k_bytes = Botan::BigInt::encode(key);
	std::uint32_t unknown = 0;

	Botan::SHA_160 hasher;
	hasher.update(packet.username);
	hasher.update((Botan::byte*)&unknown, sizeof(unknown));
	hasher.update((Botan::byte*)&packet.seed, sizeof(packet.seed));
	hasher.update((Botan::byte*)&ctx->auth_seed, sizeof(ctx->auth_seed));
	hasher.update(k_bytes);
	Botan::SecureVector<Botan::byte> calc_hash = hasher.final();

	if(calc_hash != packet.digest) {
		LOG_DEBUG_GLOB << "Received bad digest from " << packet.username << LOG_ASYNC;
		send_auth_result(ctx, protocol::ResultCode::AUTH_BAD_SERVER_PROOF);
		return;
	}

	ctx->connection->set_authenticated(key);
	ctx->account_name = packet.username;

	auto auth_success = [packet](ClientContext* ctx) {
		LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

		++test;
		ctx->auth_status = AuthStatus::SUCCESS;
		send_auth_result(ctx, protocol::ResultCode::AUTH_OK);
		send_addon_data(ctx, packet);
		ctx->handler->state_update(ClientState::CHARACTER_LIST);
	};

	/*
	* Note: MaNGOS claims you need a full auth packet for the initial AUTH_WAIT_QUEUE
	* but that doesn't seem to be true - if this bugs out, check that out
	*/
	if(test > 0) {
		auto self(ctx->connection->shared_from_this());

		queue_service_temp->enqueue(self, [auth_success, ctx, packet, self]() {
			ctx->connection->socket().get_io_service().dispatch([self, ctx, auth_success, packet]() {
				LOG_DEBUG_GLOB << packet.username << " removed from queue" << LOG_ASYNC;
				auth_success(ctx);
			});
		});

		LOG_DEBUG_GLOB << packet.username << " added to queue" << LOG_ASYNC;
		ctx->handler->state_update(ClientState::IN_QUEUE);
		return;
	}

	auth_success(ctx);
}

void send_auth_result(ClientContext* ctx, protocol::ResultCode result) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	// not convinced that this packet is correct, apart from for AUTH_OK
	protocol::SMSG_AUTH_RESPONSE response;
	response.result = result;
	ctx->connection->send(response);
}

} // unnamed

namespace authentication {

void enter(ClientContext* ctx) {
	send_auth_challenge(ctx);
}

void update(ClientContext* ctx) {
	switch(ctx->header->opcode) {
		case protocol::ClientOpcodes::CMSG_AUTH_SESSION:
			handle_authentication(ctx);
			break;
		//default:
			//ctx->state = ClientState::UNEXPECTED_PACKET;
	}
}

void exit(ClientContext* ctx) {
	// don't care
}

}} // authentication, ember