/*
 * Copyright (c) 2018 - 2025 Ember
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

namespace ember::gateway {

[[nodiscard]]
bool ClientHandler::deserialise(protocol::is_packet auto& packet, BinaryStream& stream) {
	if(packet->read_from_stream(stream)) {
		if(stream.read_limit() != stream.total_read()) {
			LOG_DEBUG_ASYNC(
				logger_, "Skipping superfluous stream data in message {} from {}",
				protocol::to_string(packet.opcode), client_identify()
			);

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
	switch(stream.state()) {
		case BinaryStream::State::READ_LIMIT_ERR:
			LOG_DEBUG_ASYNC(
				logger_, "Deserialisation of {} failed, skipping any remaining data",
				protocol::to_string(packet.opcode)
			);

			stream.skip(stream.read_limit() - stream.total_read());
			break;
		case BinaryStream::State::BUFF_LIMIT_ERR:
			LOG_ERROR_ASYNC(
				logger_, "Message framing lost for {} from {}",
				protocol::to_string(packet.opcode), client_identify()
			);

			close();
			break;
		default:
			LOG_ERROR_ASYNC(
				logger_, "Deserialisation failed, stream has not errored or unhandled error for {} from {}",
				protocol::to_string(packet.opcode), client_identify()
			);

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

} // gateway, ember