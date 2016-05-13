/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientHandler.h"
#include "ClientConnection.h"
#include <spark/SafeBinaryStream.h>
#include <spark/temp/Account_generated.h>
#include <botan/bigint.h>
#include <botan/sha160.h>

#include "temp.h" // todo

#undef ERROR // temp

namespace em = ember::messaging;

namespace ember {

void ClientHandler::start() {
	auth_state_.send_auth_challenge();
}

void ClientHandler::handle_packet(protocol::ClientHeader header, spark::Buffer& buffer) {
	header_ = &header;

	// handle ping & keep-alive as special cases
	switch(header.opcode) {
		case protocol::ClientOpcodes::CMSG_PING:
			handle_ping(buffer);
			return;
		case protocol::ClientOpcodes::CMSG_KEEP_ALIVE: // no response required
			return;
	}

	ClientState saved_state = state_;

	switch(state_) {
		case ClientState::AUTHENTICATING:
			auth_state_.update(header, buffer);
			break;
		case ClientState::IN_QUEUE:
			state_ = ClientState::UNEXPECTED_PACKET;
			break;
		case ClientState::CHARACTER_LIST:
			handle_character_list(buffer);
			break;
		case ClientState::IN_WORLD:
			handle_in_world(buffer);
			break;
	}

	switch(state_) {
		case ClientState::UNEXPECTED_PACKET:
			LOG_DEBUG_FILTER(logger_, LF_NETWORK)
				<< to_string(saved_state) << " state received unexpected opcode "
				<< protocol::to_string(header.opcode) << ", skipping" << LOG_ASYNC;
			buffer.skip(header.size - sizeof(header.opcode));
			state_ = saved_state; // restore previous state
			break;
		case ClientState::REQUEST_CLOSE:
			connection_.close_session();
			break;
	}
}

bool ClientHandler::packet_deserialise(protocol::Packet& packet, spark::Buffer& buffer) {
	spark::SafeBinaryStream stream(buffer);

	if(packet.read_from_stream(stream) != protocol::Packet::State::DONE) {
		LOG_DEBUG_FILTER(logger_, LF_NETWORK)
			<< "Parsing of " << protocol::to_string(header_->opcode)
			<< " failed" << LOG_ASYNC;

		//if(close_on_failure) {
			connection_.close_session();
		//}
		
		return false;
	}

	return true;
}

void ClientHandler::handle_ping(spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	protocol::CMSG_PING packet;

	if(!packet_deserialise(packet, buffer)) {
		return;
	}

	auto response = std::make_shared<protocol::SMSG_PONG>();
	connection_.latency(packet.latency);
	response->sequence_id = packet.sequence_id;
	connection_.send(protocol::ServerOpcodes::SMSG_PONG, response);
}

void ClientHandler::send_character_list_fail() {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	auto response = std::make_shared<protocol::SMSG_CHAR_CREATE>();
	response->result = protocol::ResultCode::AUTH_UNAVAILABLE;
	connection_.send(protocol::ServerOpcodes::SMSG_CHAR_CREATE, response);
}

void ClientHandler::send_character_list(std::vector<Character> characters) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	auto response = std::make_shared<protocol::SMSG_CHAR_ENUM>();
	response->characters = characters;
	connection_.send(protocol::ServerOpcodes::SMSG_CHAR_ENUM, response);
}

void ClientHandler::handle_char_enum(spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	auto self = connection_.shared_from_this();

	char_serv_temp->retrieve_characters(account_name_,
		[this, self](em::character::Status status, std::vector<Character> characters) {
			if(status == em::character::Status::OK) {
				send_character_list(characters);
			} else {
				send_character_list_fail();
			}
	});
}

void ClientHandler::send_character_delete() {
	auto response = std::make_shared<protocol::SMSG_CHAR_CREATE>();
	response->result = protocol::ResultCode::CHAR_DELETE_SUCCESS;
	connection_.send(protocol::ServerOpcodes::SMSG_CHAR_DELETE, response);
}

void ClientHandler::send_character_create() {
	auto response = std::make_shared<protocol::SMSG_CHAR_CREATE>();
	response->result = protocol::ResultCode::CHAR_CREATE_SUCCESS;
	connection_.send(protocol::ServerOpcodes::SMSG_CHAR_CREATE, response);
}

void ClientHandler::handle_char_create(spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	protocol::CMSG_CHAR_CREATE packet;

	if(!packet_deserialise(packet, buffer)) {
		return;
	}

	auto self = connection_.shared_from_this();

	char_serv_temp->create_character(account_name_, *packet.character,
		[this, self](em::character::Status status) {
			//if(status == em::character::Status::OK) {
				send_character_create();
			//}
	});
}

void ClientHandler::handle_char_delete(spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	protocol::CMSG_CHAR_DELETE packet;

	if(!packet_deserialise(packet, buffer)) {
		return;
	}

	auto self = connection_.shared_from_this();

	char_serv_temp->delete_character(account_name_, packet.id,
		[this, self](em::character::Status status) {
			if(status == em::character::Status::OK) {
				send_character_delete();
			}
		}
	);
}

void ClientHandler::handle_login(spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

	protocol::CMSG_PLAYER_LOGIN packet;

	if(!packet_deserialise(packet, buffer)) {
		return;
	}

	/*auto response = std::make_shared<protocol::SMSG_CHARACTER_LOGIN_FAILED>();
	response->reason = 1;
	connection_.send(protocol::ServerOpcodes::SMSG_CHARACTER_LOGIN_FAILED, response);*/
}

void ClientHandler::handle_character_list(spark::Buffer& buffer) {
	switch(header_->opcode) {
		case protocol::ClientOpcodes::CMSG_CHAR_ENUM:
			handle_char_enum(buffer);
			break;
		case protocol::ClientOpcodes::CMSG_CHAR_CREATE:
			handle_char_create(buffer);
			break;
		case protocol::ClientOpcodes::CMSG_CHAR_DELETE:
			handle_char_delete(buffer);
			break;
		case protocol::ClientOpcodes::CMSG_PLAYER_LOGIN:
			handle_login(buffer);
			break;
		/*case protocol::ClientOpcodes::CMSG_CHAR_RENAME:
			handle_char_rename(buffer);
			break;*/
		// case enter world
	}
}

void ClientHandler::handle_in_world(spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

}

ClientHandler::~ClientHandler() {
	switch(state_) {
		case ClientState::IN_QUEUE:
			queue_service_temp->dequeue(connection_.shared_from_this());
			break;
		case ClientState::CHARACTER_LIST:
		case ClientState::IN_WORLD:
			queue_service_temp->decrement();
			break;
	}
}

} // ember