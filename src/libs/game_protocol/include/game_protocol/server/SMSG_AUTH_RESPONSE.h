/*
 * Copyright (c) 2015 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <game_protocol/ResultCodes.h>
#include <boost/endian/arithmetic.hpp>
#include <cstdint>
#include <cstddef>

namespace ember::protocol {

namespace be = boost::endian;

class SMSG_AUTH_RESPONSE final : public ServerPacket {
	State state_ = State::INITIAL;

public:
	SMSG_AUTH_RESPONSE() : ServerPacket(protocol::ServerOpcodes::SMSG_AUTH_RESPONSE) { }

	Result result;
	be::little_uint32_at queue_position = 0;
	be::little_uint32_at billing_time = 0;
	be::little_uint8_at billing_flags = 0;
	be::little_uint32_at billing_rested = 0;

	State read_from_stream(spark::BinaryStream& stream) override try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		stream >> result;

		if(result == Result::AUTH_WAIT_QUEUE) {
			stream >> queue_position;
		}

		if(result == Result::AUTH_OK) {
			stream >> billing_time;
			stream >> billing_flags;
			stream >> billing_rested;
		}

		return (state_ = State::DONE);
	} catch(spark::exception&) {
		return State::ERRORED;
	}

	void write_to_stream(spark::BinaryStream& stream) const override {
		stream << result;

		if(result == Result::AUTH_WAIT_QUEUE) {
			stream << queue_position;
		}

		if(result == Result::AUTH_OK) {
			stream << billing_time;
			stream << billing_flags;
			stream << billing_rested;
		}
	}
};

} // protocol, ember
