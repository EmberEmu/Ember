/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Concepts.h>
#include <protocol/StreamResult.h>
#include <expected>

namespace ember::protocol {

[[nodiscard]] StreamResult deserialise(is_packet auto& packet, le_stream auto& stream) {
	if(packet.read_payload_from_stream(stream)) {
		if(stream.read_limit() == stream.total_read()) {
			return StreamResult::success;
		} else {
			return StreamResult::unprocessed_data;
		}
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
			return StreamResult::overrun;
		case spark::io::StreamState::buffer_limit_error:
			return StreamResult::framing_lost;
		default:
			return StreamResult::stream_error;
	}
}

template<is_packet PacketType>
std::expected<PacketType, StreamResult> deserialise(le_stream auto& stream) {
	PacketType packet;
	const auto result = deserialise(packet, stream);
	return result? packet : std::unexpected(result);
}

} // protocol, ember