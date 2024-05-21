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
#include <botan/secmem.h>
#include <boost/assert.hpp>
#include <boost/endian/arithmetic.hpp>
#include <array>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::server {

namespace be = boost::endian;

class LoginProof final : public Packet {
	static const std::size_t HEADER_LENGTH = 2;
	static const std::size_t BODY_LENGTH = 24;
	static const std::size_t PROOF_LENGTH = 20;

	State state_ = State::INITIAL;

	void read_head(spark::BinaryStream& stream) {
		stream >> opcode;
		stream >> result;
	}

	void read_body(spark::BinaryStream& stream) {
		// no need to keep reading - the other fields aren't set
		if(result != grunt::Result::SUCCESS) {
			state_ = State::DONE;
			return;
		}

		// must be CMD_AUTH_LOGIN_PROOF, so read the rest of the packet
		if(stream.size() < BODY_LENGTH) {
			state_ = State::CALL_AGAIN;
			return;
		}
		
		std::uint8_t m2_buff[PROOF_LENGTH];
		stream.get(m2_buff, PROOF_LENGTH);
		std::reverse(std::begin(m2_buff), std::end(m2_buff));
		M2 = Botan::BigInt(m2_buff, PROOF_LENGTH);

		stream >> survey_id;
	}

public:
	LoginProof() : Packet(Opcode::CMD_AUTH_LOGON_PROOF) {}

	Result result;
	Botan::BigInt M2;
	be::little_uint32_t survey_id = 0;

	State read_from_stream(spark::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < HEADER_LENGTH) {
			return State::CALL_AGAIN;
		}

		switch(state_) {
			case State::INITIAL:
				read_head(stream);
				[[fallthrough]];
			case State::CALL_AGAIN:
				read_body(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}
		
		return state_;
	}

	void write_to_stream(spark::BinaryStream& stream) const override {
		stream << opcode;
		stream << result;

		// no need to stream the rest of the members
		if(result != grunt::Result::SUCCESS && result != grunt::Result::SUCCESS_SURVEY) {
			return;
		}

		std::array<std::uint8_t, PROOF_LENGTH> bytes{};
		Botan::BigInt::encode_1363(bytes.data(), bytes.size(), M2);
		stream.put(bytes.rbegin(), bytes.rend());
		stream << survey_id;
	
	}
};

} // server, grunt, ember