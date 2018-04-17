/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Event.h"
#include "states/ClientContext.h"
#include <game_protocol/Packets.h>
#include <spark/buffers/Buffer.h>
#include <spark/buffers/BinaryStream.h>
#include <logger/Logging.h>
#include <shared/ClientUUID.h>
#include <boost/asio/steady_timer.hpp>
#include <boost/uuid/uuid.hpp>
#include <chrono>
#include <memory>

namespace ember {

class ClientConnection;

class ClientHandler final {
	ClientConnection& connection_;
	ClientContext context_;
	const ClientUUID uuid_;
	log::Logger* logger_;
	boost::asio::steady_timer timer_;

	std::string client_identify();
	void handle_ping(spark::Buffer& buffer);

public:
	ClientHandler(ClientConnection& connection, ClientUUID uuid, log::Logger* logger,
	              boost::asio::io_service& service);

	void state_update(ClientState new_state);
	void packet_skip(spark::Buffer& buffer, protocol::ClientOpcode opcode);

	template<typename Packet_t>
	bool packet_deserialise(Packet_t& packet, spark::Buffer& buffer) {
		spark::BinaryStream stream(buffer, context_.msg_size);

		Packet_t::Opcode_t opcode;
		stream >> opcode;

		if(packet->read_from_stream(stream) != protocol::State::DONE) {
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
	};

	void handle_message(spark::Buffer& buffer, protocol::SizeType size);
	void handle_event(const Event* event);
	void handle_event(std::unique_ptr<const Event> event);

	void start();
	void stop();

	void start_timer(const std::chrono::milliseconds& time);
	void stop_timer();

	const ClientUUID& uuid() const {
		return uuid_;
	}
};

} // ember