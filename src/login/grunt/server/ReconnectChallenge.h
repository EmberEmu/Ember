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
#include "../ResultCodes.h"
#include <botan/bigint.h>
#include <botan/auto_rng.h>
#include <botan/secmem.h>
#include <cstdint>
#include <cstddef>

namespace ember { namespace grunt { namespace server {

class ReconnectChallenge final : public Packet {
	const static std::size_t WIRE_LENGTH = 34;
	const static std::size_t RAND_LENGTH = 16;

	State state_ = State::INITIAL;

public:
	Opcode opcode;
	ResultCode result;
	std::array<Botan::byte, RAND_LENGTH> salt;
	std::array<Botan::byte, RAND_LENGTH> rand2; // probably another salt for client integrity checking, todo

	State read_from_stream(spark::SafeBinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}
		
		stream >> opcode;
		stream >> result;
		stream.get(salt.data(), salt.size());
		stream.get(rand2.data(), rand2.size());

		return (state_ = State::DONE);
	}

	void write_to_stream(spark::BinaryStream& stream) const override {
		BOOST_ASSERT_MSG(rand.size() == RAND_LENGTH, "CMD_RECONNECT_CHALLENGE rand != RAND_LENGTH");

		stream << opcode;
		stream << result;
		stream.put(salt.data(), salt.size());
		stream.put(rand2.data(), rand2.size());
	}
};

}}} // server, grunt, ember