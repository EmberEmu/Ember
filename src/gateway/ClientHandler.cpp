/*
 * Copyright (c) 2016 - 2018 Ember
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
#include <shared/util/EnumHelper.h>
#include <game_protocol/Packets.h>
#include <spark/BinaryStream.h>

namespace ember {

void ClientHandler::start() {
	Locator::dispatcher()->register_handler(this);
	enter_states[context_.state](&context_);
}

void ClientHandler::stop() {
	Locator::dispatcher()->remove_handler(this);
	state_update(ClientState::SESSION_CLOSED);
}

void ClientHandler::handle_message(spark::Buffer& buffer, protocol::SizeType size) {
	protocol::ClientOpcode opcode;
	buffer.copy(&opcode, sizeof(opcode));

	context_.buffer = &buffer;
	context_.msg_size = size;

	// handle ping & keep-alive as special cases
	switch(opcode) {
		case protocol::ClientOpcode::CMSG_PING:
			handle_ping(buffer);
			return;
		case protocol::ClientOpcode::CMSG_KEEP_ALIVE: // no response required
			return;
		default:
			break;
	}

	update_packet[context_.state](&context_, opcode);
}

void ClientHandler::handle_event(const Event* event) {
	update_event[context_.state](&context_, event);
}

void ClientHandler::handle_event(std::unique_ptr<const Event> event) {
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

[[nodiscard]] bool ClientHandler::packet_deserialise(protocol::Packet& packet, spark::Buffer& buffer) {
	spark::BinaryStream stream(buffer, context_.msg_size);

	protocol::ClientOpcode opcode;
	stream >> opcode;

	if(packet.read_from_stream(stream) != protocol::Packet::State::DONE) {
		const auto state = stream.state();

		/*
		 * READ_LIMIT_ERR:
		 * Deserialisation failed due to an attempt to read beyond the
		 * message boundary. This could be caused by an error in the message
		 * definition or a malicious client spoofing the size in the
		 * header. We can recover from this.
		 * 
		 * BUFF_LIMIT_ERR:
		 * Deserialisation failed due to a buffer underrun - this should never
		 * happen and message framing has likely been lost if this ever
		 * occurs. Don't try to recover.
		 */
		if(state == spark::BinaryStream::State::READ_LIMIT_ERR) {
			LOG_DEBUG_FILTER(logger_, LF_NETWORK)
				<< "Deserialisation of "
				<< protocol::to_string(opcode)
				<< " failed, skipping any remaining data" << LOG_ASYNC;

			stream.skip(stream.read_limit() - stream.total_read());
		} else if(state == spark::BinaryStream::State::BUFF_LIMIT_ERR) {
			LOG_ERROR_FILTER(logger_, LF_NETWORK)
				<< "Message framing lost at "
				<< protocol::to_string(opcode)
				<< " from " << client_identify() << LOG_ASYNC;

			connection_.close_session();
		} else {
			LOG_ERROR_FILTER(logger_, LF_NETWORK)
				<< "Deserialisation failed but stream has not errored for "
				<< protocol::to_string(opcode)
				<< " from " << client_identify() << LOG_ASYNC;

			connection_.close_session();
		}

		return false;
	}

	if(stream.read_limit() != stream.total_read()) {
		LOG_DEBUG_FILTER(logger_, LF_NETWORK)
			<< "Skipping superfluous stream data in message "
			<< protocol::to_string(opcode)
			<< " from " << client_identify() << LOG_ASYNC;

		stream.skip(stream.read_limit() - stream.total_read());
	}

	return true;
}

void ClientHandler::packet_skip(spark::Buffer& buffer, protocol::ClientOpcode opcode) {
	LOG_DEBUG_FILTER(logger_, LF_NETWORK)
		<< ClientState_to_string(context_.state) << " requested skip of packet "
		<< protocol::to_string(opcode) << " (" << util::enum_value(opcode) << ") from "
		<< client_identify() << LOG_ASYNC;

	buffer.skip(context_.msg_size);
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

void ClientHandler::start_timer(const std::chrono::milliseconds& time) {
	timer_.expires_from_now(time);
	timer_.async_wait([this](const boost::system::error_code& ec) {
		if(!ec) {
			const Event event {EventType::TIMER_EXPIRED};
			handle_event(&event);
		}
	});
}

void ClientHandler::stop_timer() {
	timer_.cancel();
}

/*
 * Helper that decides whether to print the IP address or username
 * and IP address in log outputs, based on whether authentication
 * has completed
 */
std::string ClientHandler::client_identify() {
	if(context_.auth_status == AuthStatus::SUCCESS) {
		return context_.account_name + "(" + context_.connection->remote_address() + ")";
	} else {
		return context_.connection->remote_address();
	}
}

ClientHandler::ClientHandler(ClientConnection& connection, ClientUUID uuid, log::Logger* logger,
                             boost::asio::io_service& service)
                             : context_{}, connection_(connection), logger_(logger), uuid_(uuid),
                               timer_(service) { 
	context_.state = context_.prev_state = ClientState::AUTHENTICATING;
	context_.connection = &connection_;
	context_.handler = this;
}

} // ember