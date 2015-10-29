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
	Botan::SecureVector<Botan::byte> rand;
	std::uint64_t unknown = 0;
	std::uint64_t unknown2 = 0;

	State read_from_stream(spark::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}
		
		stream >> opcode;
		stream >> result;

		rand.resize(RAND_LENGTH);
		stream.get(rand.begin(), RAND_LENGTH);

		stream >> unknown;
		stream >> unknown2;

		return (state_ = State::DONE);
	}

	void write_to_stream(spark::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(rand.size() == RAND_LENGTH, "SMSG_RECONNECT_CHALLENGE rand != RAND_LEN");

		stream << opcode;
		stream << result;
		stream.put(rand.begin(), rand.size());
		stream << unknown;
		stream << unknown2;
	}
};

}}} // server, grunt, ember