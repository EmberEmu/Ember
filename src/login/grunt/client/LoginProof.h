/*
 * Copyright (c) 2015 - 2021 Ember
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
#include <botan/secmem.h>
#include <gsl/gsl_util>
#include <array>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::client {

class LoginProof final : public Packet {
	enum class ReadState {
		READ_KEY_DATA, READ_PIN_TYPE,
		READ_PIN_DATA, DONE
	} read_state_ = ReadState::READ_KEY_DATA;

	State state_ = State::INITIAL;

	static const std::size_t WIRE_LENGTH = 74; 
	static const unsigned int A_LENGTH = 32;
	static const unsigned int M1_LENGTH = 20;
	static const unsigned int SHA1_LENGTH = 20;
	static const std::uint8_t PIN_SALT_LENGTH = 16;
	static const std::uint8_t PIN_HASH_LENGTH = 20;

	std::uint8_t key_count_ = 0;

	void read_body(spark::BinaryStream& stream) {
		stream >> opcode;

		std::array<std::uint8_t, A_LENGTH> a_buff;
		stream.get(a_buff.data(), a_buff.size());
		std::reverse(std::begin(a_buff), std::end(a_buff));
		A = Botan::BigInt(a_buff.data(), a_buff.size());

		std::array<std::uint8_t, M1_LENGTH> m1_buff;
		stream.get(m1_buff.data(), m1_buff.size());
		std::reverse(std::begin(m1_buff), std::end(m1_buff));
		M1 = Botan::BigInt(m1_buff.data(), m1_buff.size());

		stream.get(client_checksum.data(), client_checksum.size());
		stream >> key_count_;
	}

	bool read_security_type(spark::BinaryStream& stream) {
		if(stream.size() < sizeof(two_factor_auth)) {
			return false;
		}

		stream >> two_factor_auth;

		if(two_factor_auth) {
			read_state_ = ReadState::READ_PIN_DATA;
		} else {
			read_state_ = ReadState::DONE;
		}

		return true;
	}

	bool read_pin_data(spark::BinaryStream& stream) {
		if(stream.size() < (pin_salt.size() + pin_hash.size())) {
			return false;
		}

		stream.get(pin_salt.data(), pin_salt.size());
		stream.get(pin_hash.data(), pin_hash.size());

		read_state_ = ReadState::DONE;
		return true;
	}

	bool read_key_data(spark::BinaryStream& stream) {
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
			stream.get(data.product.data(), data.product.size());
			stream.get(data.hash.data(), data.hash.size());
			keys.emplace_back(data);
		}

		read_state_ = ReadState::READ_PIN_TYPE;
		return true;
	}

public:
	LoginProof() : Packet(Opcode::CMD_AUTH_LOGON_PROOF) {}

	Botan::BigInt A;
	Botan::BigInt M1;
	bool two_factor_auth = 0;
	std::array<std::uint8_t, SHA1_LENGTH> client_checksum;
	std::array<std::uint8_t, PIN_SALT_LENGTH> pin_salt;
	std::array<std::uint8_t, PIN_HASH_LENGTH> pin_hash;
	std::vector<KeyData> keys;

	void read_optional_data(spark::BinaryStream& stream) {
		bool continue_read = true;

		while(continue_read) {
			switch(read_state_) {
				case ReadState::READ_KEY_DATA:
					continue_read = read_key_data(stream);
					break;
				case ReadState::READ_PIN_TYPE:
					continue_read = read_security_type(stream);
					break;
				case ReadState::READ_PIN_DATA:
					continue_read = read_pin_data(stream);
					break;
				case ReadState::DONE:
					continue_read = false;
					break;
			}
		}

		state_ = (read_state_ == ReadState::DONE)? State::DONE : State::CALL_AGAIN;
	}

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
				read_optional_data(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}

		return state_;
	}

	void write_to_stream(spark::BinaryStream& stream) const override {
		stream << opcode;

		auto bytes = Botan::BigInt::encode_1363(A, A_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.data(), bytes.size());

		bytes = Botan::BigInt::encode_1363(M1, M1_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.data(), bytes.size());

		stream.put(client_checksum.data(), client_checksum.size());

		stream << gsl::narrow<std::uint8_t>(keys.size());

		for(auto& key : keys) {
			stream << key.len;
			stream << key.pub_value;
			stream.put(key.product.data(), key.product.size());
			stream.put(key.hash.data(), key.hash.size());
		}

		stream << two_factor_auth;

		if(two_factor_auth) {
			stream.put(pin_salt.data(), pin_salt.size());
			stream.put(pin_hash.data(), pin_hash.size());
		}
	}
};

} // client, grunt, ember