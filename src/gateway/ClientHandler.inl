/*
 * Copyright (c) 2018 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ConnectionDefines.h"
#include <protocol/Concepts.h>
#include <logger/Logger.h>
#include <optional>

bool ClientHandler::deserialise(protocol::is_packet auto& packet, BinaryStream& stream) {
	if(packet->read_from_stream(stream) == protocol::State::DONE) {
		
		if(stream.read_limit() != stream.total_read()) {
			LOG_DEBUG_FILTER(logger_, LF_NETWORK)
				<< "Skipping superfluous stream data in message "
				<< protocol::to_string(packet.opcode)
				<< " from " << client_identify() << LOG_ASYNC;

			stream.skip(stream.read_limit() - stream.total_read());
		}

		return true;
	}	

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
	const auto state = stream.state();

	if(state == BinaryStream::State::READ_LIMIT_ERR) {
		LOG_DEBUG_FILTER(logger_, LF_NETWORK)
			<< "Deserialisation of "
			<< protocol::to_string(packet.opcode)
			<< " failed, skipping any remaining data" << LOG_ASYNC;

		stream.skip(stream.read_limit() - stream.total_read());
	} else if(state == BinaryStream::State::BUFF_LIMIT_ERR) {
		LOG_ERROR_FILTER(logger_, LF_NETWORK)
			<< "Message framing lost at "
			<< protocol::to_string(packet.opcode)
			<< " from " << client_identify() << LOG_ASYNC;

		close();
	} else {
		LOG_ERROR_FILTER(logger_, LF_NETWORK)
			<< "Deserialisation failed but stream has not errored for "
			<< protocol::to_string(packet.opcode)
			<< " from " << client_identify() << LOG_ASYNC;

		close();
	}

	return false;
}

template<protocol::is_packet T>
std::optional<T> ClientHandler::deserialise(BinaryStream& stream) {
	T packet;
	const auto result = deserialise(packet, stream);
	
	if(result) {
		return packet;
	} else {
		return std::nullopt;
	}
}