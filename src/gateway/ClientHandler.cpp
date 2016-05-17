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

#undef ERROR // temp

namespace ember {

void ClientHandler::start() {
	enter_states[context_.state](&context_);
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

	ClientState prev_state = context_.state;

	update_states[context_.state](&context_);

	if(prev_state != context_.state) {
		exit_states[context_.state](&context_);
		enter_states[context_.state](&context_);
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

void ClientHandler::handle_in_world(spark::Buffer& buffer) {
	LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

}

ClientHandler::ClientHandler(ClientConnection& connection, log::Logger* logger)
                             : context_{}, connection_(connection), logger_(logger) { 
	context_.state = ClientState::AUTHENTICATING;
	context_.connection = &connection_;
	context_.handler = this;
}

ClientHandler::~ClientHandler() {
	exit_states[context_.state](&context_);
}

} // ember