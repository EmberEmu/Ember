/*
 * Copyright (c) 2018 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientLogHelper.h"
#include "ClientConnection.h"
#include "ConnectionDefines.h"
#include <protocol/Concepts.h>
#include <logger/Logger.h>
#include <optional>

namespace ember::realm {

// this is a thunk to allow for outgoing logging without polluting the connection
inline void ClientHandler::send(is_packet auto& packet) {
	PACKET_TRACE(context_, "<- {}", packet.opcode);
	connection_->send(packet);
}

[[nodiscard]]
bool ClientHandler::deserialise(is_packet auto& packet, BinaryStream& stream) {
	if(auto result = packet.read_payload_from_stream(stream); result) {
		if(stream.read_limit() != stream.total_read()) {
			LOG_DEBUG(logger_, "Skipping unprocessed data in message {} from {}",
				packet.opcode, client_identify()
			);

			stream.skip(stream.read_limit() - stream.total_read());
		}

		return true;
	} else {
		LOG_TRACE(logger_, "Unexpected stream result: {} ({})", result.value(), result.what());
	}

	/*
	 * read_limit_error:
	 * Deserialisation failed due to an attempt to read beyond the
	 * message boundary. This could be caused by an error in the message
	 * definition or a malicious client spoofing the size in the
	 * header. We can recover from this.
	 * 
	 * buffer_limit_error:
	 * Deserialisation failed due to a buffer underrun (tried to read more data
	 * than the buffer currently holds) - this should never happen and message
	 * framing has likely been lost if this ever occurs. Don't try to recover.
	 */
	switch(stream.state()) {
		case spark::io::StreamState::read_limit_error:
			LOG_DEBUG(
				logger_, "Deserialisation of {} failed, skipping any remaining data",
				protocol::to_string(packet.opcode)
			);

			stream.skip(stream.read_limit() - stream.total_read());
			break;
		case spark::io::StreamState::buffer_limit_error:
			LOG_ERROR(
				logger_, "Message framing lost for {} from {}",
				protocol::to_string(packet.opcode), client_identify()
			);

			close();
			break;
		default:
			LOG_ERROR(logger_,
				"Deserialisation failed, stream state {}, unhandled error for {} from {}",
				std::to_underlying(stream.state()), packet.opcode, client_identify()
			);

			close();
	}

	return false;
}

template<is_packet PacketType>
std::optional<PacketType> ClientHandler::deserialise(BinaryStream& stream) {
	PacketType packet;
	const auto result = deserialise(packet, stream);
	return result? packet : std::nullopt;
}

} // realm, ember