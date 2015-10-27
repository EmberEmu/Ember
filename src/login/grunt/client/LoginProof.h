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
	const unsigned int A_LENGTH = 32;
	const unsigned int M1_LENGTH = 20;
	const unsigned int M2_LENGTH = 20;

public:
	static const std::size_t WIRE_LENGTH = 75;

	Opcode opcode;
	Botan::BigInt A;
	Botan::BigInt M1;
	Botan::BigInt crc_hash;
	std::uint8_t key_count;
	std::uint8_t unknown;

	State deserialise(spark::Buffer& buffer) override {
		spark::BinaryStream stream(buffer);
		
		stream >> opcode;

		Botan::byte buff[32];

		stream.get(buff, A_LENGTH); // A
		std::reverse(std::begin(buff), std::end(buff));
		A = Botan::BigInt(buff, A_LENGTH);

		stream.get(buff, M1_LENGTH); // M1
		std::reverse(std::begin(buff), std::end(buff));
		M1 = Botan::BigInt(buff, M1_LENGTH);

		stream.get(buff, M2_LENGTH); // crc_hash
		std::reverse(std::begin(buff), std::end(buff));
		crc_hash = Botan::BigInt(buff, M2_LENGTH);

		stream >> key_count;
		stream >> unknown;

		return State::DONE;
	}
};

}}} // client, grunt, ember