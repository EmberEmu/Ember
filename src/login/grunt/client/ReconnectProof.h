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
	const unsigned int R1_LENGTH = 16;
	const unsigned int R2_LENGTH = 20;
	const unsigned int R3_LENGTH = 20;

public:
	static const std::size_t WIRE_LENGTH = 58;

	Opcode opcode;
	Botan::BigInt R1;
	Botan::BigInt R2;
	Botan::BigInt R3;
	std::uint8_t key_count;

	State deserialise(spark::Buffer& buffer) override {
		spark::BinaryStream stream(buffer);
		Botan::byte buff[20];

		stream >> opcode;

		stream.get(buff, R1_LENGTH); // R1
		std::reverse(std::begin(buff), std::end(buff));
		R1 = Botan::BigInt(buff, R1_LENGTH);

		stream.get(buff, R2_LENGTH); // R2
		std::reverse(std::begin(buff), std::end(buff));
		R1 = Botan::BigInt(buff, R2_LENGTH);

		stream.get(buff, R3_LENGTH); // R3
		std::reverse(std::begin(buff), std::end(buff));
		R1 = Botan::BigInt(buff, R3_LENGTH);

		stream >> key_count;

		return State::DONE;
	}
};

}}} // client, grunt, ember