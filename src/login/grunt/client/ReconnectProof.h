/*
 * Copyright (c) 2015 Ember
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
	std::array<Botan::byte, 16> salt;
	std::array<Botan::byte, 20> proof;
	std::array<Botan::byte, 20> client_checksum;
	std::uint8_t key_count;

	State read_from_stream(spark::SafeBinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		stream >> opcode;
		stream.get(salt.data(), salt.size());
		stream.get(proof.data(), proof.size());
		stream.get(client_checksum.data(), client_checksum.size());
		stream >> key_count;

		return (state_ = State::DONE);
	}

	void write_to_stream(spark::BinaryStream& stream) const override {
		stream << opcode;
		stream.put(salt.data(), salt.size());
		stream.put(proof.data(), proof.size());
		stream.put(client_checksum.data(), client_checksum.size());
		stream << key_count;
	}
};

}}} // client, grunt, ember