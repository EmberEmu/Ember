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
#include <botan/botan.h>
#include <cstdint>

namespace ember { namespace grunt { namespace client {

class LoginProof : public Packet {
	State state_ = State::INITIAL;

	static const std::size_t WIRE_LENGTH = 75; 

	static const unsigned int A_LENGTH = 32;
	static const unsigned int M1_LENGTH = 20;
	static const unsigned int M2_LENGTH = 20;

public:
	Opcode opcode;
	Botan::BigInt A;
	Botan::BigInt M1;
	Botan::BigInt crc_hash;
	std::uint8_t key_count;
	std::uint8_t unknown;

	State read_from_stream(spark::BinaryStream& stream) override {
		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		stream >> opcode;

		// could just use one buffer but this is safer from silly mistakes
		Botan::byte a_buff[A_LENGTH];
		stream.get(a_buff, A_LENGTH);
		std::reverse(std::begin(a_buff), std::end(a_buff));
		A = Botan::BigInt(a_buff, A_LENGTH);

		Botan::byte m1_buff[M1_LENGTH];
		stream.get(m1_buff, M1_LENGTH);
		std::reverse(std::begin(m1_buff), std::end(m1_buff));
		M1 = Botan::BigInt(m1_buff, M1_LENGTH);

		Botan::byte m2_buff[M2_LENGTH];
		stream.get(m2_buff, M2_LENGTH); 
		std::reverse(std::begin(m2_buff), std::end(m2_buff));
		crc_hash = Botan::BigInt(m2_buff, M2_LENGTH);

		stream >> key_count;
		stream >> unknown;

		return State::DONE;
	}
};

}}} // client, grunt, ember