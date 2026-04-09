/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CharacterList.h"
#include "ClientContext.h"
#include "../CharacterClient.h"
#include "../ClientHandler.h"
#include "../ConfigStore.h"
#include "../CharacterClient.h"
#include "../ClientConnection.h"
#include "../ClientLogHelper.h"
#include "../Events.h"
#include "../EventDispatcher.h"
#include "../FilterTypes.h"
#include "../RealmService.h"
#include "../RealmQueue.h"
#include <logger/Logger.h>
#include <protocol/Opcodes.h>
#include <protocol/Packets.h>
#include <shared/utility/UTF8String.h>
#include <memory>
#include <vector>

using namespace std::chrono_literals;

namespace ember::realm::character_list {

void handle_timeout(ClientContext& ctx);

void send_character_list_fail(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	// displays an error dialogue on the client
	protocol::smsg_char_create response;
	response->result = protocol::Result::char_list_failed;
	ctx.handler.send(response);
}

void send_character_list(ClientContext& ctx, std::vector<Character> characters) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::smsg_char_enum response;
	response->characters = std::move(characters);
	ctx.handler.send(response);
}

void send_character_rename(ClientContext& ctx, const CharRenameResponse& res) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::smsg_char_rename response;
	response->result = res.result;
	response->id = res.character_id;
	response->name = res.name;
	ctx.handler.send(response);
}

void character_rename(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::cmsg_char_rename packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	const auto& uuid = ctx.handler.uuid();
	auto& dispatcher = ctx.dispatcher;

	ctx.character_rpc.rename_character(ctx.client_id->id, packet->id, packet->name,
	                                  [dispatcher, uuid](auto result, auto id, const auto& name) {
		CharRenameResponse event(result, id, name);
		dispatcher.post(uuid, std::move(event));
	});
}

void character_enumerate(const ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	const auto& uuid = ctx.handler.uuid();
	auto& dispatcher = ctx.dispatcher;

	ctx.character_rpc.retrieve_characters(ctx.client_id->id,
		[dispatcher, uuid](auto status, auto characters) {
			CharEnumResponse event(status, std::move(characters));
			dispatcher.post(uuid, std::move(event));
		}
	);
}

void character_enumerate_completion(ClientContext& ctx, const CharEnumResponse& res) {
	LOG_TRACE(ctx.logger, log_func);

	if(res.status == rpc::Character::Status::ok) {
		send_character_list(ctx, res.characters);
	} else {
		send_character_list_fail(ctx);
	}
}

void send_character_delete(ClientContext& ctx, const CharDeleteResponse& res) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::smsg_char_delete response;
	response->result = res.result;
	ctx.handler.send(response);
}

void send_character_create(ClientContext& ctx, const CharCreateResponse& res) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::smsg_char_create response;
	response->result = res.result;
	ctx.handler.send(response);
}

void character_create(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::cmsg_char_create packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	const auto& uuid = ctx.handler.uuid();
	auto& dispatcher = ctx.dispatcher;

	ctx.character_rpc.create_character(ctx.client_id->id, packet->character, [dispatcher, uuid](auto result) {
		dispatcher.post(uuid, CharCreateResponse(result));
	});
}

void character_delete(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::cmsg_char_delete packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	const auto& uuid = ctx.handler.uuid();
	auto& dispatcher = ctx.dispatcher;

	ctx.character_rpc.delete_character(ctx.client_id->id, packet->id, [dispatcher, uuid](auto result) {
		dispatcher.post(uuid, CharDeleteResponse(result));
	});
}

void player_login(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::cmsg_player_login packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	ctx.dispatcher.post(
		ctx.handler.uuid(), PlayerLogin(packet->character_id)
	);

	ctx.handler.state_update(ClientState::cs_world_enter);
}

void handle_timeout(ClientContext& ctx) {
	CLIENT_DEBUG(ctx, "Character list timed out");
	ctx.handler.close();
}

void enter(ClientContext& ctx) {
	const auto& config = ctx.cfg_store.config_tls();

	if(auto timeout = config.char_list_timeout; timeout != 0s) {
		ctx.timer.start(timeout);
	}
}

/*
 * If smsg_auth_response is sent when the game is on the loading screen,
 * it gets ignored and will require a restart, so we'll send the expected
 * login failed packet instead
 * 
 * cmsg_cancel_trade is triggered by the client unregistering its handler when
 * it leaves the world, so we'll just ignore it
 */
void offline_response(ClientContext& ctx, protocol::ClientOpcode opcode) {
	if(opcode == protocol::ClientOpcode::cmsg_player_login) {
		protocol::smsg_character_login_failed response;
		response->reason = protocol::Result::char_login_no_world;
		ctx.handler.send(response);
	} else if(opcode != protocol::ClientOpcode::cmsg_cancel_trade) {
		protocol::smsg_auth_response response;
		response->result = protocol::Result::realm_list_realm_not_found;
		ctx.handler.send(response);
	}

	ctx.handler.skip(*ctx.stream);
}

void handle_packet(ClientContext& ctx, protocol::ClientOpcode opcode) {
	using enum protocol::ClientOpcode;

	if(!ctx.realm_rpc.online()) {
		offline_response(ctx, opcode);
		return;
	}

	switch(opcode) {
		case cmsg_char_enum:
			character_enumerate(ctx);
			break;
		case cmsg_char_create:
			character_create(ctx);
			break;
		case cmsg_char_delete:
			character_delete(ctx);
			break;
		case cmsg_char_rename:
			character_rename(ctx);
			break;
		case cmsg_player_login:
			player_login(ctx);
			break;
		default:
			break;
	}
}

void handle_event(ClientContext& ctx, const Event& event) {
	using enum EventType;

	switch(event.type) {
		case timer_expired:
			handle_timeout(ctx);
			break;
		case char_create_response:
			send_character_create(ctx, event.as<CharCreateResponse>());
			break;
		case char_delete_response:
			send_character_delete(ctx, event.as<CharDeleteResponse>());
			break;
		case char_enum_response:
			character_enumerate_completion(ctx, event.as<CharEnumResponse>());
			break;
		case char_rename_response:
			send_character_rename(ctx, event.as<CharRenameResponse>());
			break;
		default:
			break;
	}
}

void exit(ClientContext& ctx) {
	ctx.timer.cancel();

	if(ctx.state == ClientState::cs_session_closed) {
		//--test;
		ctx.queue.free_slot();
	}
}

} // character_list, realm, ember