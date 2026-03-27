/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Opcodes.h"
#include "../Packet.h"
#include "../Exceptions.h"
#include <boost/assert.hpp>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::server {

class TransferData final : public Packet {
	static const std::size_t wire_length = 1;
	State state_ = State::initial;

public:
	static const std::uint16_t MAX_CHUNK_SIZE = 65535;

	TransferData()
		: Packet(Opcode::cmd_xfer_data) {}

	std::uint16_t size = 0;
	std::array<std::byte, MAX_CHUNK_SIZE> chunk;

	State read_from_stream(PacketStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(stream.size() < wire_length) {
			return State::call_again;
		}

		stream >> opcode;
		stream >> size;

		return (state_ = State::done);
	}

	void write_to_stream(PacketStream& stream) const override {
		stream << opcode;
		stream << size;
		stream.put(chunk.data(), size);
	}
};

} // client, grunt, ember