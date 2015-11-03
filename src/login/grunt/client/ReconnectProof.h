/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Opcodes.h"
#include "../Packet.h"
#include "../Exceptions.h"
#include <boost/assert.hpp>
#include <botan/bigint.h>
#include <botan/secmem.h>
#include <array>
#include <cstdint>
#include <cstddef>

namespace ember { namespace grunt { namespace client {

class ReconnectProof final : public Packet {
	static const std::size_t WIRE_LENGTH = 58;
	State state_ = State::INITIAL;

public:
	Opcode opcode;
	std::array<Botan::byte, 16> R1;
	std::array<Botan::byte, 20> R2;
	std::array<Botan::byte, 20> R3;
	std::uint8_t key_count;

	State read_from_stream(spark::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		stream >> opcode;
		stream.get(R1.data(), R1.size());
		stream.get(R2.data(), R2.size());
		stream.get(R3.data(), R3.size());
		stream >> key_count;

		return (state_ = State::DONE);
	}

	void write_to_stream(spark::BinaryStream& stream) override {
		stream << opcode;
		stream.put(R1.data(), R1.size());
		stream.put(R2.data(), R2.size());
		stream.put(R3.data(), R3.size());
		stream << key_count;
	}
};

}}} // client, grunt, ember