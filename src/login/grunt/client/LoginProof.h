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

class LoginProof final : public Packet {
	State state_ = State::INITIAL;

	static const std::size_t WIRE_LENGTH = 75; 
	static const unsigned int A_LENGTH = 32;
	static const unsigned int M1_LENGTH = 20;
	static const unsigned int CRC_LENGTH = 20;
	static const std::uint8_t PIN_SALT_LENGTH = 16;
	static const std::uint8_t PIN_HASH_LENGTH = 20;

	void read_body(spark::BinaryStream& stream) {
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

		Botan::byte crc_buff[CRC_LENGTH];
		stream.get(crc_buff, CRC_LENGTH);
		std::reverse(std::begin(crc_buff), std::end(crc_buff));
		crc_hash = Botan::BigInt(crc_buff, CRC_LENGTH);

		stream >> key_count;
		stream >> static_cast<TwoFactorSecurity>(security);
	}

	void read_pin_data(spark::BinaryStream& stream) {
		switch(security) {
			case TwoFactorSecurity::NONE:
				state_ = State::DONE;
				break;
			case TwoFactorSecurity::PIN:
				if(stream.size() >= (pin_salt.size() + pin_hash.size())) {
					stream.get(pin_salt.data(), pin_salt.size());
					stream.get(pin_hash.data(), pin_hash.size());
					state_ = State::DONE;
				} else {
					state_ = State::CALL_AGAIN;
				}
				break;
			default:
				throw grunt::bad_packet("Unknown security method from client");
		}
	}

public:
	enum class TwoFactorSecurity : std::uint8_t {
		NONE, PIN
	};

	Opcode opcode;
	Botan::BigInt A;
	Botan::BigInt M1;
	Botan::BigInt crc_hash;
	std::uint8_t key_count;
	TwoFactorSecurity security;
	std::array<std::uint8_t, PIN_SALT_LENGTH> pin_salt;
	std::array<std::uint8_t, PIN_HASH_LENGTH> pin_hash;

	State read_from_stream(spark::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		switch(state_) {
			case State::INITIAL:
				read_body(stream);
				[[fallthrough]];
			case State::CALL_AGAIN:
				read_pin_data(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}

		return state_;
	}

	void write_to_stream(spark::BinaryStream& stream) const override {
		stream << opcode;

		Botan::SecureVector<Botan::byte> bytes = Botan::BigInt::encode_1363(A, A_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.begin(), bytes.size());

		bytes = Botan::BigInt::encode_1363(M1, M1_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.begin(), bytes.size());

		bytes = Botan::BigInt::encode_1363(crc_hash, CRC_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.begin(), bytes.size());

		stream << key_count;
		stream << security;

		if(security == TwoFactorSecurity::PIN) {
			stream.put(pin_salt.data(), pin_salt.size());
			stream.put(pin_hash.data(), pin_hash.size());
		}
	}
};

}}} // client, grunt, ember