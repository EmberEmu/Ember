/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <game_protocol/ResultCodes.h>
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstddef>

namespace ember::protocol {

namespace be = boost::endian;

class SMSG_CHARACTER_LOGIN_FAILED final : public ServerPacket {
	State state_ = State::INITIAL;

public:
	SMSG_CHARACTER_LOGIN_FAILED() : ServerPacket(protocol::ServerOpcodes::SMSG_CHARACTER_LOGIN_FAILED) { }

	std::uint8_t reason;

	State read_from_stream(spark::SafeBinaryStream& stream) override try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		//stream >> std::uint8_t1;

		return (state_ = State::DONE);
	} catch(spark::buffer_underrun&) {
		return State::ERRORED;
	}

	void write_to_stream(spark::SafeBinaryStream& stream) const override {
		stream << reason;
	}
};

} // protocol, ember
