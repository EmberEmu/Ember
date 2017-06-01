/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstddef>

namespace ember::protocol {

namespace be = boost::endian;

class SMSG_AUTH_CHALLENGE final : public ServerPacket {
	State state_ = State::INITIAL;

public:
	SMSG_AUTH_CHALLENGE() : ServerPacket(protocol::ServerOpcodes::SMSG_AUTH_CHALLENGE) { }

	std::uint32_t seed;

	State read_from_stream(spark::SafeBinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		stream >> seed;
		be::little_to_native_inplace(seed);

		return state_;
	}

	void write_to_stream(spark::SafeBinaryStream& stream) const override {
		stream << be::native_to_little(seed);
	}
};

} // protocol, ember
