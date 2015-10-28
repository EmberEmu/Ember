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

class ReconnectProof final : public Packet {
	static const std::size_t WIRE_LENGTH = 58;
	static const std::size_t R1_LENGTH = 16;
	static const std::size_t R2_LENGTH = 20;
	static const std::size_t R3_LENGTH = 20;
	State state_ = State::INITIAL;

public:
	Opcode opcode;
	Botan::BigInt R1;
	Botan::BigInt R2;
	Botan::BigInt R3;
	std::uint8_t key_count;

	State read_from_stream(spark::BinaryStream& stream) override {
		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		stream >> opcode;

		// could just use one buffer but this is safer from silly mistakes
		Botan::byte r1_buff[R1_LENGTH];
		stream.get(r1_buff, R1_LENGTH);
		std::reverse(std::begin(r1_buff), std::end(r1_buff));
		R1 = Botan::BigInt(r1_buff, R1_LENGTH);

		Botan::byte r2_buff[R2_LENGTH];
		stream.get(r2_buff, R2_LENGTH);
		std::reverse(std::begin(r2_buff), std::end(r2_buff));
		R1 = Botan::BigInt(r2_buff, R2_LENGTH);

		Botan::byte r3_buff[R3_LENGTH];
		stream.get(r3_buff, R3_LENGTH);
		std::reverse(std::begin(r3_buff), std::end(r3_buff));
		R1 = Botan::BigInt(r3_buff, R3_LENGTH);

		stream >> key_count;

		return State::DONE;
	}
};

}}} // client, grunt, ember