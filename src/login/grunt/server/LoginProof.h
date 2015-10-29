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
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstddef>

namespace ember { namespace grunt { namespace server {

namespace be = boost::endian;

class LoginProof : public Packet {
	static const std::size_t WIRE_LENGTH = 26;
	static const std::size_t PROOF_LENGTH = 20;

	State state_ = State::INITIAL;

public:
	Opcode opcode;
	ResultCode result;
	Botan::BigInt M2;
	std::uint32_t account_flags;

	State read_from_stream(spark::BinaryStream& stream) {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}
		
		stream >> opcode;
		stream >> result;

		Botan::byte m2_buff[PROOF_LENGTH];
		stream.get(m2_buff, PROOF_LENGTH);
		std::reverse(std::begin(m2_buff), std::end(m2_buff));
		M2 = Botan::BigInt(m2_buff, PROOF_LENGTH);

		stream >> account_flags;
		be::little_to_native_inplace(account_flags);

		return (state_ = State::DONE);
	}

	void write_to_stream(spark::BinaryStream& stream) {
		stream << opcode;
		stream << result;

		Botan::SecureVector<Botan::byte> bytes = Botan::BigInt::encode_1363(M2, PROOF_LENGTH);
		std::reverse(std::begin(bytes), std::end(bytes));
		stream.put(bytes.begin(), bytes.size());

		stream << be::native_to_little(account_flags);
	}
};

}}} // server, grunt, ember