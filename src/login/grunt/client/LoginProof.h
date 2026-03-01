/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Opcodes.h"
#include "../Packet.h"
#include "../Exceptions.h"
#include "../KeyData.h"
#include <boost/assert.hpp>
#include <boost/endian/arithmetic.hpp>
#include <botan/bigint.h>
#include <gsl/narrow>
#include <array>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::client {

class LoginProof final : public Packet {
	enum class ReadState {
		key_data,
		pin_type,
		pin_data,
		done
	} read_state_ = ReadState::key_data;

	State state_ = State::initial;

	static const std::size_t WIRE_LENGTH = 74; 
	static const unsigned int A_LENGTH = 32;
	static const unsigned int M1_LENGTH = 20;
	static const unsigned int SHA1_LENGTH = 20;
	static const std::uint8_t PIN_SALT_LENGTH = 16;
	static const std::uint8_t PIN_HASH_LENGTH = 20;

	std::uint8_t key_count_ = 0;

	void read_body(spark::io::pmr::BinaryStream& stream) {
		stream >> opcode;

		std::array<std::uint8_t, A_LENGTH> a_buff;
		stream >> a_buff;
		std::ranges::reverse(a_buff);
		A = Botan::BigInt(a_buff);

		std::array<std::uint8_t, M1_LENGTH> m1_buff;
		stream >> m1_buff;
		std::ranges::reverse(m1_buff);
		M1 = Botan::BigInt(m1_buff);

		stream >> client_checksum;
		stream >> key_count_;
	}

	bool read_security_type(spark::io::pmr::BinaryStream& stream) {
		if(stream.size() < sizeof(two_factor_auth)) {
			return false;
		}

		stream >> two_factor_auth;

		if(two_factor_auth) {
			read_state_ = ReadState::pin_data;
		} else {
			read_state_ = ReadState::done;
		}

		return true;
	}

	bool read_pin_data(spark::io::pmr::BinaryStream& stream) {
		if(stream.size() < (pin_salt.size() + pin_hash.size())) {
			return false;
		}

		stream >> pin_salt;
		stream >> pin_hash;

		read_state_ = ReadState::done;
		return true;
	}

	bool read_key_data(spark::io::pmr::BinaryStream& stream) {
		// could use a macro to take care of this - not using sizeof(KeyData) to avoid having to #pragma pack
		auto key_data_size = sizeof(KeyData::product) + sizeof(KeyData::pub_value) 
		                     + sizeof(KeyData::len) + sizeof(KeyData::hash);
		key_data_size *= key_count_;

		if(stream.size() < key_data_size) {
			return false;
		}

		for(auto i = 0; i < key_count_; ++i) {
			KeyData data;
			stream >> data.len;
			stream >> data.pub_value;
			stream >> data.product;
			stream >> data.hash;
			keys.emplace_back(data);
		}

		read_state_ = ReadState::pin_type;
		return true;
	}

public:
	LoginProof()
		: Packet(Opcode::cmd_auth_logon_proof) {}

	Botan::BigInt A;
	Botan::BigInt M1;
	bool two_factor_auth = 0;
	std::array<std::uint8_t, SHA1_LENGTH> client_checksum;
	std::array<std::uint8_t, PIN_SALT_LENGTH> pin_salt;
	std::array<std::uint8_t, PIN_HASH_LENGTH> pin_hash;
	std::vector<KeyData> keys;

	void read_optional_data(spark::io::pmr::BinaryStream& stream) {
		bool continue_read = true;

		while(continue_read) {
			switch(read_state_) {
				case ReadState::key_data:
					continue_read = read_key_data(stream);
					break;
				case ReadState::pin_type:
					continue_read = read_security_type(stream);
					break;
				case ReadState::pin_data:
					continue_read = read_pin_data(stream);
					break;
				case ReadState::done:
					continue_read = false;
					break;
			}
		}

		state_ = (read_state_ == ReadState::done)? State::done : State::call_again;
	}

	State read_from_stream(spark::io::pmr::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(state_ == State::initial && stream.size() < WIRE_LENGTH) {
			return State::call_again;
		}

		switch(state_) {
			case State::initial:
				read_body(stream);
				[[fallthrough]];
			case State::call_again:
				read_optional_data(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}

		return state_;
	}

	void write_to_stream(spark::io::pmr::BinaryStream& stream) const override {
		stream << opcode;

		std::array<std::uint8_t, A_LENGTH> a_bytes;
		A.serialize_to(a_bytes);
		stream.put(a_bytes.rbegin(), a_bytes.rend());

		std::array<std::uint8_t, M1_LENGTH> m1_bytes;
		M1.serialize_to(m1_bytes);
		stream.put(m1_bytes.rbegin(), m1_bytes.rend());

		stream << client_checksum;

		stream << gsl::narrow<std::uint8_t>(keys.size());

		for(auto& key : keys) {
			stream << key.len;
			stream << key.pub_value;
			stream << key.product;
			stream << key.hash;
		}

		stream << two_factor_auth;

		if(two_factor_auth) {
			stream << pin_salt;
			stream << pin_hash;
		}
	}
};

} // client, grunt, ember