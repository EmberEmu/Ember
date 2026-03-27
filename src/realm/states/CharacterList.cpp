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
#include "../RealmQueue.h"
#include "../CharacterClient.h"
#include "../ClientConnection.h"
#include "../EventDispatcher.h"
#include "../FilterTypes.h"
#include "../ClientLogHelper.h"
#include "../Events.h"
#include <logger/Logger.h>
#include <protocol/Packets.h>
#include <protocol/Opcodes.h>
#include <shared/utility/UTF8String.h>
#include <memory>
#include <vector>

using namespace std::chrono_literals;

namespace ember::realm::character_list {

namespace {

void handle_timeout(ClientContext& ctx);

void send_character_list_fail(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	// displays an error dialogue on the client
	protocol::smsg_char_create response;
	response->result = protocol::Result::char_list_failed;
	ctx.connection->send(response);
}

void send_character_list(ClientContext& ctx, std::vector<Character> characters) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::smsg_char_enum response;
	response->characters = std::move(characters);
	ctx.connection->send(response);
}

void send_character_rename(ClientContext& ctx, const CharRenameResponse* res) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::smsg_char_rename response;
	response->result = res->result;
	response->id = res->character_id;
	response->name = res->name;
	ctx.connection->send(response);
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
		dispatcher.post_event(uuid, std::move(event));
	});
}

void character_enumerate(const ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	const auto& uuid = ctx.handler.uuid();
	auto& dispatcher = ctx.dispatcher;

	ctx.character_rpc.retrieve_characters(ctx.client_id->id,
		[dispatcher, uuid](auto status, auto characters) {
			CharEnumResponse event(status, std::move(characters));
			dispatcher.post_event(uuid, std::move(event));
		}
	);
}

void character_enumerate_completion(ClientContext& ctx, const CharEnumResponse* event) {
	LOG_TRACE(ctx.logger, log_func);

	if(event->status == rpc::Character::Status::ok) {
		send_character_list(ctx, event->characters);
	} else {
		send_character_list_fail(ctx);
	}
}

void send_character_delete(ClientContext& ctx, const CharDeleteResponse* res) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::smsg_char_delete response;
	response->result = res->result;
	ctx.connection->send(response);
}

void send_character_create(ClientContext& ctx, const CharCreateResponse* res) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::smsg_char_create response;
	response->result = res->result;
	ctx.connection->send(response);
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
		dispatcher.post_event(uuid, CharCreateResponse(result));
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
		dispatcher.post_event(uuid, CharDeleteResponse(result));
	});
}

void player_login(ClientContext& ctx) {
	LOG_TRACE(ctx.logger, log_func);

	protocol::cmsg_player_login packet;

	if(!ctx.handler.deserialise(packet, *ctx.stream)) {
		return;
	}

	ctx.dispatcher.post_event(
		ctx.handler.uuid(), PlayerLogin(packet->character_id)
	);

	ctx.handler.state_update(ClientState::cs_world_enter);
}

void handle_timeout(ClientContext& ctx) {
	CLIENT_DEBUG(ctx.logger, ctx) << "Character list timed out" << LOG_ASYNC;
	ctx.handler.close();
}

} // unnamed

void enter(ClientContext& ctx) {
	const auto& config = ctx.cfg_store.config_tls();

	if(auto timeout = config.char_list_timeout; timeout != 0s) {
		ctx.handler.start_timer(timeout);
	}
}

void handle_packet(ClientContext& ctx, protocol::ClientOpcode opcode) {
	switch(opcode) {
		case protocol::ClientOpcode::cmsg_char_enum:
			character_enumerate(ctx);
			break;
		case protocol::ClientOpcode::cmsg_char_create:
			character_create(ctx);
			break;
		case protocol::ClientOpcode::cmsg_char_delete:
			character_delete(ctx);
			break;
		case protocol::ClientOpcode::cmsg_char_rename:
			character_rename(ctx);
			break;
		case protocol::ClientOpcode::cmsg_player_login:
			player_login(ctx);
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
		case EventType::char_create_response:
			send_character_create(ctx, static_cast<const CharCreateResponse*>(event));
			break;
		case EventType::char_delete_response:
			send_character_delete(ctx, static_cast<const CharDeleteResponse*>(event));
			break;
		case EventType::char_enum_response:
			character_enumerate_completion(ctx, static_cast<const CharEnumResponse*>(event));
			break;
		case EventType::char_rename_response:
			send_character_rename(ctx, static_cast<const CharRenameResponse*>(event));
			break;
		default:
			break;
	}
}

void exit(ClientContext& ctx) {
	ctx.handler.cancel_timer();

	if(ctx.state == ClientState::cs_session_closed) {
		//--test;
		ctx.queue.free_slot();
	}
}

} // character_list, realm, ember