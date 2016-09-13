/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Opcodes.h>
#include <spark/SafeBinaryStream.h>
#include <cstdint>

namespace ember { namespace protocol {

struct Packet {
	enum class State {
		INITIAL, CALL_AGAIN, DONE, ERRORED
	};

	virtual State read_from_stream(spark::SafeBinaryStream& stream) = 0;
	virtual void write_to_stream(spark::SafeBinaryStream& stream) const = 0;
	virtual ~Packet() = default;
};

struct ServerPacket : public Packet {
	ServerOpcodes opcode;
	ServerPacket(protocol::ServerOpcodes opcode) : opcode(opcode) { }
};

// todo, overload this properly
inline spark::SafeBinaryStream& operator<<(spark::SafeBinaryStream& out, const ServerPacket& packet) {
	packet.write_to_stream(out);
	return out;
}

//inline spark::SafeBinaryStream& operator>>(spark::SafeBinaryStream& in, Packet& packet) {
//	packet.read_from_stream(in); // todo, stream error states
//	return in;
//}

}} // client, grunt, ember