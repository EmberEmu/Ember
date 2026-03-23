/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientHandler.h"
#include "ClientConnection.h"
#include "ClientLogHelper.h"
#include "EventDispatcher.h"
#include "FilterTypes.h"
#include "Locator.h"
#include "states/StateJumpTables.h"
#include <logger/Logger.h>
#include <protocol/Packets.h>
#include <format>
#include <utility>

namespace ember::realm {

void ClientHandler::start() {
	Locator::dispatcher()->register_handler(this);
	enter_state[context_.state](context_);
}

void ClientHandler::stop() {
	Locator::dispatcher()->remove_handler(this);
	state_update(ClientState::cs_session_closed);
}

void ClientHandler::close() {
	connection_.close_session();
}

void ClientHandler::handle_message(StaticBuffer& buffer, const protocol::SizeType msg_size) {
	BinaryStream stream(buffer, msg_size);
	context_.stream = &stream;
	protocol::ClientOpcode opcode;
	stream >> opcode;

	CLIENT_TRACE(logger_, context_) << " -> " << protocol::to_string(opcode) << LOG_ASYNC;

	// handle ping & keep-alive as special cases
	switch(opcode) {
		case protocol::ClientOpcode::cmsg_ping:
			handle_ping(stream);
			return;
		case protocol::ClientOpcode::cmsg_keep_alive: // no response required
			return;
		default:
			break;
	}

	update_packet[context_.state](context_, opcode);
}

void ClientHandler::handle_event(const Event* event) {
	update_event[context_.state](context_, event);
}

void ClientHandler::handle_event(std::unique_ptr<const Event> event) {
	update_event[context_.state](context_, event.get());
}

void ClientHandler::state_update(ClientState new_state) {
	CLIENT_DEBUG(logger_, context_)
		<< "State change, " << ClientState_to_string(context_.state)
		<< " => " << ClientState_to_string(new_state) << LOG_SYNC;

	context_.prev_state = context_.state;
	context_.state = new_state;
	exit_state[context_.prev_state](context_);
	enter_state[context_.state](context_);
}

void ClientHandler::skip(BinaryStream& stream) {
	CLIENT_TRACE(logger_, context_)
		<< ClientState_to_string(context_.state)
		<< " skipping message" << LOG_ASYNC;

	stream.skip(stream.read_limit() - stream.total_read());
}

void ClientHandler::handle_ping(BinaryStream& stream) {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	message_view<protocol::cmsg_ping> packet;

	if(!deserialise(packet, stream)) {
		return;
	}

	protocol::smsg_pong response;
	response->sequence_id = packet->sequence_id;
	connection_.latency(packet->latency);
	connection_.send(response);
}

void ClientHandler::start_timer(const std::chrono::milliseconds& time) {
	timer_.expires_after(time);

	timer_.async_wait([uuid = uuid_](const boost::system::error_code& ec) {
		if(!ec) {
			Event event { EventType::timer_expired };
			Locator::dispatcher()->post_event(uuid, event);
		}
	});
}

void ClientHandler::cancel_timer() {
	timer_.cancel_one();
}

/*
 * Helper that decides whether to print the IP address or username
 * and IP address in log outputs, based on whether authentication
 * has completed
 */
std::string_view ClientHandler::client_identify() const {	
	if(context_.client_id) {
		if(client_id_ext_.empty()) {
			client_id_ext_ = std::format(
				"{} ({}, {})", client_id_, context_.client_id->username, context_.client_id->id
			);
		}

		return client_id_ext_;
	}

	if(client_id_.empty()) [[unlikely]] {
		client_id_ = connection_.remote_address();
	}

	return client_id_;
}

ClientHandler::ClientHandler(ClientConnection& connection, ClientRef uuid,
							 executor executor, log::Logger& logger)
	: context_ {
		.handler = *this,
		.connection = connection,
		.logger = logger,
		.state = ClientState::cs_authenticating,
		.prev_state = ClientState::cs_authenticating,
	  },
	  connection_(connection),
	  logger_(logger),
	  uuid_(uuid),
	  timer_(executor) {
}

ClientHandler::~ClientHandler() {
	if(context_.state != ClientState::cs_session_closed) {
		close();
	}
}

} // realm, ember