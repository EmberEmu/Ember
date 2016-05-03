/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Opcodes.h>
#include <game_protocol/Packet.h>
#include <game_protocol/ResultCodes.h>
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstddef>

namespace ember { namespace protocol {

namespace be = boost::endian;

class SMSG_AUTH_CHALLENGE final : public Packet {
	static const std::size_t WIRE_LENGTH = 6;

	State state_ = State::INITIAL;

public:
	ServerOpcodes opcode = ServerOpcodes::SMSG_AUTH_CHALLENGE;
	std::uint32_t seed = 3211;

	State read_from_stream(spark::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		stream >> opcode;
		stream >> seed;
		//be::big_to_native_inplace(opcode);
		//be::little_to_native_inplace(seed);

		return state_;
	}

	void write_to_stream(spark::BinaryStream& stream) override {
		
		stream << be::native_to_big(std::uint16_t(6));
		stream << be::native_to_little(std::uint16_t(opcode));
		stream << be::native_to_little(seed);
	}
};

}} // protocol, ember