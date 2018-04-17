/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/PacketHeaders.h>
#include <spark/buffers/BinaryStream.h>
#include <gsl/gsl_util>
#include <cstdint>

namespace ember::protocol {

enum class State {
	INITIAL, CALL_AGAIN, DONE, ERRORED
};

template <typename OpT, OpT opcode, typename SizeT, typename BodyT>
struct Packet final {
	using OpcodeType = OpT;
	using SizeType = SizeT;
	using PayloadType = BodyT;

	static constexpr OpcodeType opcode = opcode;
	static constexpr std::size_t HEADER_WIRE_SIZE =
		sizeof(SizeType) + sizeof(OpcodeType);

	PayloadType payload;

	State read_from_stream(spark::BinaryStream& stream) {
		return payload.read_from_stream(stream);
	}

	void write_to_stream(spark::BinaryStream& stream) const {
		payload.write_to_stream(stream);
	}

	friend spark::BinaryStream& operator<<(spark::BinaryStream& stream, const Packet& p) {
		p.write_to_stream(stream);
		return stream;
	}

	auto* operator->() {
		return &payload;
	}

	const auto* operator->() const {
		return &payload;
	}
};

template<ServerOpcode opcode, typename Body>
struct ServerPacket {
	using Type = Packet<ServerOpcode, opcode, SizeType, Body>;
};

template<ClientOpcode opcode, typename Body>
struct ClientPacket {
	using Type = Packet<ClientOpcode, opcode, SizeType, Body>;
};

} // protocol, ember