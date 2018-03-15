/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Opcodes.h>
#include <game_protocol/PacketHeaders.h>
#include <spark/BinaryStream.h>
#include <spark/buffers/NullBuffer.h>
#include <gsl/gsl_util>
#include <cstdint>

namespace ember::protocol {

struct Packet {
	enum class State {
		INITIAL, CALL_AGAIN, DONE, ERRORED
	};

	virtual State read_from_stream(spark::BinaryStream& stream) = 0;
	virtual void write_to_stream(spark::BinaryStream& stream) const = 0;
	virtual ~Packet() = default;
};

struct ServerPacket : public Packet {
	ServerOpcodes opcode;
	ServerPacket(protocol::ServerOpcodes opcode) : opcode(opcode) { }

	ServerHeader build_header() const {
		spark::BinaryStream stream(spark::NullBuffer{});
		stream << opcode;
		write_to_stream(stream);
		return { gsl::narrow<std::uint16_t>(stream.total_write()), opcode };
	}
};

// todo, overload this properly
inline spark::BinaryStream& operator<<(spark::BinaryStream& out, const ServerPacket& packet) {
	packet.write_to_stream(out);
	return out;
}

} // protocol, ember