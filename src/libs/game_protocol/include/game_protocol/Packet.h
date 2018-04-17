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

template <typename OpcodeT, OpcodeT opcode, typename SizeType, typename BodyT>
struct Packet final {
	using Opcode_t = OpcodeT;

	static constexpr OpcodeT opcode = opcode;
	static constexpr std::size_t HEADER_WIRE_SIZE =
		sizeof(SizeType) + sizeof(OpcodeT);

	BodyT payload;

	State read_from_stream(spark::BinaryStream& stream) {
		return payload.read_from_stream(stream);
	}

	void write_to_stream(spark::BinaryStream& stream) const {
		const auto initial = stream.total_write();
		stream << SizeType{0} << opcode;
		payload.write_to_stream(stream);
		const auto written = stream.total_write() - initial;
		
		stream.write_seek(written, spark::SeekDir::SD_BACK);
		stream << gsl::narrow<SizeType>(written - sizeof(SizeType));
		stream.write_seek(written - sizeof(SizeType), spark::SeekDir::SD_FORWARD);
	}

	friend spark::BinaryStream& operator<<(spark::BinaryStream& stream, const Packet& p) {
		p.write_to_stream(stream);
		return stream;
	}

	BodyT* operator->() {
		return &payload;
	}

	const BodyT* operator->() const {
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