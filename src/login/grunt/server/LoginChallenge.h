/*
 * Copyright (c) 2015, 2016 Ember
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
#include <botan/secmem.h>
#include <boost/endian/conversion.hpp>
#include <array>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::server {

namespace be = boost::endian;

class LoginChallenge final : public Packet {
	static const std::size_t WIRE_LENGTH = 119;
	static const std::uint8_t SALT_LENGTH = 32;

	State state_ = State::INITIAL;

	void read_body(spark::SafeBinaryStream& stream) {
		stream >> opcode;
		stream >> result;
		stream >> protocol_ver;

		if(result != grunt::Result::SUCCESS) {
			state_ = State::DONE;
			return; // rest of the fields won't be sent
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
		s = Botan::BigInt(s_buff, SALT_LENGTH);

		stream.get(checksum_salt.data(), checksum_salt.size());
		stream >> two_factor_auth;
	}

	void read_pin_data(spark::SafeBinaryStream& stream) {
		if(!two_factor_auth || state_ == State::DONE) {
			return;
		}

		// does the stream hold enough bytes to complete the PIN data?
		if(stream.size() >= (pin_salt.size() + sizeof(pin_grid_seed))) {
			stream >> pin_grid_seed;
			be::little_to_native_inplace(pin_grid_seed);
			stream.get(pin_salt.data(), PIN_SALT_LENGTH);
			state_ = State::DONE;
		} else {
			state_ = State::CALL_AGAIN;
		}
	}

public:
	static const std::uint8_t PRIME_LENGTH         = 32;
	static const std::uint8_t PUB_KEY_LENGTH       = 32;
	static const std::uint8_t PIN_SALT_LENGTH      = 16;
	static const std::uint8_t CHECKSUM_SALT_LENGTH = 16;

	LoginChallenge() : Packet(Opcode::CMD_AUTH_LOGON_CHALLENGE) { }

	enum class TwoFactorSecurity : std::uint8_t {
		NONE, PIN
	};

	Result result;
	std::uint8_t protocol_ver = 0;
	Botan::BigInt B;
	std::uint8_t g_len;
	std::uint8_t g;
	std::uint8_t n_len;
	Botan::BigInt N;
	Botan::BigInt s;
	std::array<Botan::byte, CHECKSUM_SALT_LENGTH> checksum_salt;
	bool two_factor_auth = false;
	std::uint32_t pin_grid_seed;
	std::array<std::uint8_t, PIN_SALT_LENGTH> pin_salt;

	// todo - early abort (wire length change)
	State read_from_stream(spark::SafeBinaryStream& stream) override {
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
		stream << protocol_ver;
		stream << result;

		if(result != grunt::Result::SUCCESS) {
			return; // don't send the rest of the fields
		}
		
		Botan::secure_vector<Botan::byte> bytes = Botan::BigInt::encode_1363(B, PUB_KEY_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.data(), bytes.size());

		stream << g_len;
		stream << g;
		stream << n_len;

		bytes = Botan::BigInt::encode_1363(N, PRIME_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.data(), bytes.size());

		bytes = Botan::BigInt::encode_1363(s, SALT_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.data(), bytes.size());

		stream.put(checksum_salt.data(), checksum_salt.size());
		stream << two_factor_auth;

		if(two_factor_auth) {
			stream << be::native_to_little(pin_grid_seed);
			stream.put(pin_salt.data(), pin_salt.size());
		}
	}
};

} // server, grunt, ember