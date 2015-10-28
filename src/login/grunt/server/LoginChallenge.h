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
#include <botan/secmem.h>
#include <cstdint>
#include <cstddef>

namespace ember { namespace grunt { namespace server {

class LoginChallenge : public Packet {
	static const std::size_t WIRE_LENGTH = 119;
	static const std::uint8_t PRIME_LENGTH = 32;
	static const std::uint8_t PUB_KEY_LENGTH = 32;
	static const std::uint8_t SALT_LENGTH = 32;

	State state_ = State::INITIAL;

public:
	Opcode opcode;
	ResultCode error;
	std::uint8_t unk2;
	Botan::BigInt B;
	std::uint8_t g_len;
	std::uint8_t g;
	std::uint8_t n_len;
	Botan::BigInt N;
	Botan::BigInt s;
	std::uint8_t unk3[16];
	std::uint8_t unk4;

	// todo - early abort
	State read_from_stream(spark::BinaryStream& stream) {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		stream >> opcode;
		stream >> error;
		
		Botan::byte b_buff[PUB_KEY_LENGTH];
		stream.get(b_buff, PUB_KEY_LENGTH);
		std::reverse(std::begin(b_buff), std::end(b_buff));
		B = Botan::BigInt(b_buff, PUB_KEY_LENGTH);

		stream >> g_len;
		stream >> g;
		stream >> n_len;
		
		Botan::byte n_buff[PRIME_LENGTH];
		stream.get(n_buff, PRIME_LENGTH);
		std::reverse(std::begin(n_buff), std::end(n_buff));
		N = Botan::BigInt(n_buff, PRIME_LENGTH);

		Botan::byte s_buff[SALT_LENGTH];
		stream.get(s_buff, SALT_LENGTH);
		std::reverse(std::begin(s_buff), std::end(s_buff));
		s = Botan::BigInt(b_buff, SALT_LENGTH);

		stream.get(unk3, sizeof(unk3));
		stream >> unk4;

		return (state_ = State::DONE);
	}

	void write_to_stream(spark::BinaryStream& stream) {
		stream << opcode;
		stream << error;
		
		Botan::SecureVector<Botan::byte> bytes = Botan::BigInt::encode_1363(B, PUB_KEY_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.begin(), bytes.size());

		stream << g_len;
		stream << g;
		stream << n_len;

		bytes = Botan::BigInt::encode_1363(N, PRIME_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.begin(), bytes.size());

		bytes = Botan::BigInt::encode_1363(s, SALT_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.begin(), bytes.size());

		stream.put(unk3, sizeof(unk3));
		stream << unk4;
	}
};

}}} // server, grunt, ember