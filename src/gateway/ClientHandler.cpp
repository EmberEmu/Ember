/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientHandler.h"
#include "ClientConnection.h"
#include "states/StateLUT.h"
#include <game_protocol/Packets.h>
#include <spark/SafeBinaryStream.h>

namespace ember {

void ClientHandler::start() {
	enter_states[context_.state](&context_);
}

void ClientHandler::stop() {
	state_update(ClientState::SESSION_CLOSED);
}

void ClientHandler::handle_packet(protocol::ClientHeader header, spark::Buffer& buffer) {
	context_.header = &header;
	context_.buffer = &buffer;

	// handle ping & keep-alive as special cases
	switch(header.opcode) {
		case protocol::ClientOpcodes::CMSG_PING:
			handle_ping(buffer);
			return;
		case protocol::ClientOpcodes::CMSG_KEEP_ALIVE: // no response required
			return;
	}

	update_states[context_.state](&context_);
}

void ClientHandler::handle_event(std::shared_ptr<Event> event) {
    handle_event[context_.state](&context_, event);
}

void ClientHandler::state_update(ClientState new_state) {
	LOG_DEBUG_FILTER(logger_, LF_NETWORK) << client_identify() << ": "
		<< "State change, " << ClientState_to_string(context_.state)
		<< " => " << ClientState_to_string(new_state) << LOG_SYNC;

	context_.prev_state = context_.state;
	context_.state = new_state;
	exit_states[context_.prev_state](&context_);
	enter_states[context_.state](&context_);
}

// todo, this should go somewhere else
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

	protocol::SMSG_PONG response;
	response.sequence_id = packet.sequence_id;
	connection_.latency(packet.latency);
	connection_.send(response);
}

/*
 * Helper that decides whether to print the username or IP address in
 * log outputs, based on whether authentication has completed
 */
std::string ClientHandler::client_identify() {
	if(context_.auth_status == AuthStatus::SUCCESS) {
		return context_.account_name;
	} else {
		return context_.connection->remote_address();
	}
}

ClientHandler::ClientHandler(ClientConnection& connection, client_uuid::uuid uuid, log::Logger* logger)
                             : context_{}, connection_(connection), logger_(logger),
                               header_(nullptr), uuid_(uuid) { 
	context_.state = context_.prev_state = ClientState::AUTHENTICATING;
	context_.connection = &connection_;
	context_.handler = this;
}

} // ember