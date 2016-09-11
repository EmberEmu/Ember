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
	enum class ReadState {
		READ_KEY_DATA, READ_PIN_TYPE,
		READ_PIN_DATA, DONE
	} read_state_ = ReadState::READ_KEY_DATA;

	State state_ = State::INITIAL;

	static const std::size_t WIRE_LENGTH = 74; 
	static const unsigned int A_LENGTH = 32;
	static const unsigned int M1_LENGTH = 20;
	static const unsigned int CRC_LENGTH = 20;
	static const std::uint8_t PIN_SALT_LENGTH = 16;
	static const std::uint8_t PIN_HASH_LENGTH = 20;

	std::uint8_t key_count_ = 0;

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
		client_checksum = Botan::BigInt(crc_buff, CRC_LENGTH);

		stream >> key_count_;
	}

	bool read_security_type(spark::BinaryStream& stream) {
		if(stream.size() < sizeof(TwoFactorSecurity)) {
			return false;
		}

		stream >> static_cast<TwoFactorSecurity>(security);

		switch(security) {
			case TwoFactorSecurity::NONE:
				read_state_ = ReadState::DONE;
				break;
			case TwoFactorSecurity::PIN:
				read_state_ = ReadState::READ_PIN_DATA;
				break;
			default:
				throw grunt::bad_packet("Unknown security method from client");
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
		auto key_data_size = sizeof(KeyData::unk_1) + sizeof(KeyData::unk_2) 
		                     + sizeof(KeyData::unk_3) + sizeof(KeyData::unk_4_hash);
		key_data_size *= key_count_;

		if(stream.size() < key_data_size) {
			return false;
		}

		for(auto i = 0; i < key_count_; ++i) {
			KeyData data;
			stream >> data.unk_1;
			stream >> data.unk_2;
			stream.get(data.unk_3.data(), data.unk_3.size());
			stream.get(data.unk_4_hash.data(), data.unk_4_hash.size());
			keys.emplace_back(data);
		}

		read_state_ = ReadState::READ_PIN_TYPE;
		return true;
	}

public:
	enum class TwoFactorSecurity : std::uint8_t {
		NONE, PIN
	};

	struct KeyData {
		std::uint16_t unk_1;
		std::uint32_t unk_2;
		std::array<std::uint8_t, 4> unk_3;
		std::array<std::uint8_t, 20> unk_4_hash; // hashed with A or 'salt' if reconnect proof
	};

	Opcode opcode;
	Botan::BigInt A;
	Botan::BigInt M1;
	Botan::BigInt client_checksum;
	TwoFactorSecurity security;
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

		Botan::SecureVector<Botan::byte> bytes = Botan::BigInt::encode_1363(A, A_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.begin(), bytes.size());

		bytes = Botan::BigInt::encode_1363(M1, M1_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.begin(), bytes.size());

		bytes = Botan::BigInt::encode_1363(client_checksum, CRC_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.begin(), bytes.size());

		stream << static_cast<std::uint8_t>(keys.size());

		for(auto& key : keys) {
			stream << be::native_to_little(key.unk_1);
			stream << be::native_to_little(key.unk_2);
			stream.put(key.unk_3.data(), key.unk_3.size());
			stream.put(key.unk_4_hash.data(), key.unk_4_hash.size());
		}

		stream << security;

		if(security == TwoFactorSecurity::PIN) {
			stream.put(pin_salt.data(), pin_salt.size());
			stream.put(pin_hash.data(), pin_hash.size());
		}
	}
};

}}} // client, grunt, ember