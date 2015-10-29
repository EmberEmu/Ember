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

class LoginChallenge final : public Packet {
	static const std::size_t WIRE_LENGTH = 120;
	static const std::uint8_t SALT_LENGTH = 32;
	static const std::size_t UNKNOWN_RAND_BYTES_LENGTH = 16;

	State state_ = State::INITIAL;

public:
	static const std::uint8_t PRIME_LENGTH = 32;
	static const std::uint8_t PUB_KEY_LENGTH = 32;

	Opcode opcode;
	std::uint8_t unk1 = 0;
	ResultCode result;
	Botan::BigInt B;
	std::uint8_t g_len;
	std::uint8_t g;
	std::uint8_t n_len;
	Botan::BigInt N;
	Botan::BigInt s;
	Botan::SecureVector<Botan::byte> unk3;
	std::uint8_t unk4 = 0;

	// todo - early abort (wire length change)
	State read_from_stream(spark::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		stream >> opcode;
		stream >> unk1;
		stream >> result;

		if(result != grunt::ResultCode::SUCCESS) {
			return (state_ = State::DONE); // rest of the fields won't be sent
		}
		
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

		stream.get(unk3.begin(), unk3.size());
		stream >> unk4;

		return (state_ = State::DONE);
	}

	void write_to_stream(spark::BinaryStream& stream) override {
		stream << opcode;
		stream << unk1;
		stream << result;

		if(result != grunt::ResultCode::SUCCESS) {
			return; // don't send the rest of the fields
		}
		
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

		unk3.resize(UNKNOWN_RAND_BYTES_LENGTH);
		stream.put(unk3.begin(), unk3.size());
		stream << unk4;
	}
};

}}} // server, grunt, ember