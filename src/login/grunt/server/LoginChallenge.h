/*
 * Copyright (c) 2015 - 2024 Ember
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
#include <boost/endian/arithmetic.hpp>
#include <array>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::server {

namespace be = boost::endian;

class LoginChallenge final : public Packet {
	static const std::size_t WIRE_LENGTH = 119;
	static const std::uint8_t SALT_LENGTH = 32;

	State state_ = State::INITIAL;

	void read_body(spark::io::pmr::BinaryStream& stream) {
		stream >> opcode;
		stream >> protocol_ver;
		stream >> result;

		if(result != grunt::Result::SUCCESS) {
			state_ = State::DONE;
			return; // rest of the fields won't be sent
		}

		std::array<std::uint8_t, PUB_KEY_LENGTH> b_buff;
		stream.get(b_buff.data(), b_buff.size());
		std::ranges::reverse(b_buff);
		B = Botan::BigInt(b_buff.data(), b_buff.size());

		stream >> g_len;
		stream >> g;
		stream >> n_len;

		std::array<std::uint8_t, PRIME_LENGTH> n_buff;
		stream.get(n_buff.data(), n_buff.size());
		std::ranges::reverse(n_buff);
		N = Botan::BigInt(n_buff.data(), n_buff.size());

		std::array<std::uint8_t, SALT_LENGTH> s_buff;
		stream.get(s_buff.data(), s_buff.size());
		std::ranges::reverse(s_buff);
		s = Botan::BigInt(s_buff.data(), s_buff.size());

		stream.get(checksum_salt.data(), checksum_salt.size());
		stream >> two_factor_auth;
	}

	void read_pin_data(spark::io::pmr::BinaryStream& stream) {
		if(!two_factor_auth || state_ == State::DONE) {
			return;
		}

		// does the stream hold enough bytes to complete the PIN data?
		if(stream.size() >= (pin_salt.size() + sizeof(pin_grid_seed))) {
			stream >> pin_grid_seed;
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
	std::array<std::uint8_t, CHECKSUM_SALT_LENGTH> checksum_salt;
	bool two_factor_auth = false;
	be::little_uint32_t pin_grid_seed;
	std::array<std::uint8_t, PIN_SALT_LENGTH> pin_salt;

	// todo - early abort (wire length change)
	State read_from_stream(spark::io::pmr::BinaryStream& stream) override {
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

	void write_to_stream(spark::io::pmr::BinaryStream& stream) const override {
		stream << opcode;
		stream << protocol_ver;
		stream << result;

		if(result != grunt::Result::SUCCESS) {
			return; // don't send the rest of the fields
		}
		
		std::array<std::uint8_t, PUB_KEY_LENGTH> bytes{};
		Botan::BigInt::encode_1363(bytes.data(), bytes.size(), B);
		stream.put(bytes.rbegin(), bytes.rend());

		stream << g_len;
		stream << g;
		stream << n_len;

		static_assert(bytes.size() == PRIME_LENGTH);
		Botan::BigInt::encode_1363(bytes.data(), bytes.size(), N);
		stream.put(bytes.rbegin(), bytes.rend());

		static_assert(bytes.size() == SALT_LENGTH);
		Botan::BigInt::encode_1363(bytes.data(), bytes.size(), s);
		stream.put(bytes.rbegin(), bytes.rend());

		stream.put(checksum_salt.data(), checksum_salt.size());
		stream << two_factor_auth;

		if(two_factor_auth) {
			stream << pin_grid_seed;
			stream.put(pin_salt.data(), pin_salt.size());
		}
	}
};

} // server, grunt, ember