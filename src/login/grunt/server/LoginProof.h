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
#include <boost/assert.hpp>
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstddef>

namespace ember { namespace grunt { namespace server {

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
		if(result != grunt::ResultCode::SUCCESS) {
			state_ = State::DONE;
			return;
		}

		// must be CMD_AUTH_LOGIN_PROOF, so read the rest of the packet
		if(stream.size() < BODY_LENGTH) {
			state_ = State::CALL_AGAIN;
			return;
		}
		
		Botan::byte m2_buff[PROOF_LENGTH];
		stream.get(m2_buff, PROOF_LENGTH);
		std::reverse(std::begin(m2_buff), std::end(m2_buff));
		M2 = Botan::BigInt(m2_buff, PROOF_LENGTH);

		stream >> account_flags;
		be::little_to_native_inplace(account_flags);
	}

public:
	Opcode opcode;
	ResultCode result;
	Botan::BigInt M2;
	std::uint32_t account_flags;

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
		if(result != grunt::ResultCode::SUCCESS) {
			return;
		}

		Botan::SecureVector<Botan::byte> bytes = Botan::BigInt::encode_1363(M2, PROOF_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.begin(), bytes.size());

		stream << be::native_to_little(account_flags);
	}
};

}}} // server, grunt, ember