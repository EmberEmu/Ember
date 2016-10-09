/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientHandler.h"
#include "ClientConnection.h"
#include "Locator.h"
#include "EventDispatcher.h"
#include "states/StateLUT.h"
#include <game_protocol/Packets.h>
#include <spark/SafeBinaryStream.h>

namespace ember {

void ClientHandler::start() {
	Locator::dispatcher()->register_handler(this);
	enter_states[context_.state](&context_);
}

void ClientHandler::stop() {
	Locator::dispatcher()->remove_handler(this);
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
		default:
			break;
	}

	update_packet[context_.state](&context_);
}

void ClientHandler::handle_event(const Event* event) {
	update_event[context_.state](&context_, event);
}

void ClientHandler::handle_event(std::unique_ptr<const Event> event) {
	update_event[context_.state](&context_, event.get());
}

void ClientHandler::handle_event(std::shared_ptr<const Event> event) {
	update_event[context_.state](&context_, event.get());
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
[[nodiscard]] bool ClientHandler::packet_deserialise(protocol::Packet& packet, spark::Buffer& buffer) {
	const auto end_size = buffer.size() - context_.header->size - sizeof(protocol::ClientOpcodes);

	spark::SafeBinaryStream stream(buffer);

	if(packet.read_from_stream(stream) != protocol::Packet::State::DONE) {
		LOG_DEBUG_FILTER(logger_, LF_NETWORK)
			<< "Deserialisation of " << protocol::to_string(context_.header->opcode)
			<< " failed" << LOG_ASYNC;

		/*
		* If parsing a known message type fails, there's a good chance that
		* there's a mistake in the definition. If that's the case, check to see
		* whether we've ended up reading too much data from the buffer.
		* If so, messaging framing has likely been lost and the only thing to do
		* is to disconnect the client.
		*/
		if(buffer.size() < end_size) {
			LOG_DEBUG_FILTER(logger_, LF_NETWORK)
				<< "Message framing lost at " << protocol::to_string(context_.header->opcode)
				<< " for " << connection_.remote_address() << LOG_ASYNC;
			connection_.close_session();		
		} else {
			buffer.skip(buffer.size() - end_size);
		}

		return false;
	}

	return true;
}

// todo, this should go somewhere else
void ClientHandler::packet_skip(spark::Buffer& buffer) {
	LOG_DEBUG_FILTER(logger_, LF_NETWORK)
		<< ClientState_to_string(context_.state) << " requested skip of packet "
		<< protocol::to_string(context_.header->opcode) << " from "
		<< connection_.remote_address() << LOG_ASYNC;

	buffer.skip(context_.header->size - sizeof(protocol::ClientOpcodes));
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

ClientHandler::ClientHandler(ClientConnection& connection, ClientUUID uuid, log::Logger* logger)
                             : context_{}, connection_(connection), logger_(logger), uuid_(uuid) { 
	context_.state = context_.prev_state = ClientState::AUTHENTICATING;
	context_.connection = &connection_;
	context_.handler = this;
}

} // ember