/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CharacterList.h"
#include "../ClientHandler.h"
#include "../ClientConnection.h"
#include "../FilterTypes.h"
#include <logger/Logging.h>
#include <game_protocol/Packets.h>
#include <game_protocol/Opcodes.h>
#include <spark/temp/Account_generated.h>
#include <memory>
#include <vector>

#include "../temp.h"

namespace em = ember::messaging;

namespace ember { namespace character_list {

namespace {

void send_character_list_fail(ClientContext* ctx) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	auto response = std::make_shared<protocol::SMSG_CHAR_CREATE>();
	response->result = protocol::ResultCode::AUTH_UNAVAILABLE;
	ctx->connection->send(response);
}

void send_character_list(ClientContext* ctx, std::vector<Character> characters) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	auto response = std::make_shared<protocol::SMSG_CHAR_ENUM>();
	response->characters = characters;
	ctx->connection->send(response);
}

void handle_char_enum(ClientContext* ctx) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	auto self = ctx->connection->shared_from_this();

	char_serv_temp->retrieve_characters(ctx->account_name,
	                                    [self, ctx](em::character::Status status, std::vector<Character> characters) {
		if(status == em::character::Status::OK) {
			send_character_list(ctx, characters);
		} else {
			send_character_list_fail(ctx);
		}
	});
}

void send_character_delete(ClientContext* ctx) {
	auto response = std::make_shared<protocol::SMSG_CHAR_CREATE>();
	response->result = protocol::ResultCode::CHAR_DELETE_SUCCESS;
	ctx->connection->send(response);
}

void send_character_create(ClientContext* ctx) {
	auto response = std::make_shared<protocol::SMSG_CHAR_CREATE>();
	response->result = protocol::ResultCode::CHAR_CREATE_SUCCESS;
	ctx->connection->send(response);
}

void handle_char_create(ClientContext* ctx) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	protocol::CMSG_CHAR_CREATE packet;

	if(!ctx->handler->packet_deserialise(packet, *ctx->buffer)) {
		return;
	}

	auto self = ctx->connection->shared_from_this();

	char_serv_temp->create_character(ctx->account_name, *packet.character,
	                                 [self, ctx](em::character::Status status) {
		//if(status == em::character::Status::OK) {
			send_character_create(ctx);
		//}
	});
}

void handle_char_delete(ClientContext* ctx) {
	LOG_TRACE_FILTER_GLOB(LF_NETWORK) << __func__ << LOG_ASYNC;

	protocol::CMSG_CHAR_DELETE packet;

	if(!ctx->handler->packet_deserialise(packet, *ctx->buffer)) {
		return;
	}

	auto self = ctx->connection->shared_from_this();

	char_serv_temp->delete_character(ctx->account_name, packet.id,
	                                 [self, ctx](em::character::Status status) {
		if(status == em::character::Status::OK) {
			send_character_delete(ctx);
		}
	}
	);
}

} // unnamed

void enter(ClientContext* ctx) {
	// don't care
}

void update(ClientContext* ctx) {
	switch(ctx->header->opcode) {
		case protocol::ClientOpcodes::CMSG_CHAR_ENUM:
			handle_char_enum(ctx);
			break;
		case protocol::ClientOpcodes::CMSG_CHAR_CREATE:
			handle_char_create(ctx);
			break;
		case protocol::ClientOpcodes::CMSG_CHAR_DELETE:
			handle_char_delete(ctx);
			break;
		case protocol::ClientOpcodes::CMSG_PLAYER_LOGIN:
			//handle_login(ctx);
			break;
		/*case protocol::ClientOpcodes::CMSG_CHAR_RENAME:
			handle_char_rename(buffer);
			break;*/
		// case enter world
	}
}

void exit(ClientContext* ctx) {
	queue_service_temp->decrement();
}

}} // character_list, ember