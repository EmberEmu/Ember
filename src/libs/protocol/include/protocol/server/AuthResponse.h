/*
 * Copyright (c) 2015 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Packet.h>
#include <protocol/ResultCodes.h>
#include <spark/buffers/BinaryStream.h>
#include <boost/assert.hpp>
#include <boost/endian/arithmetic.hpp>
#include <cstdint>
#include <cstddef>

namespace ember::protocol::server {

namespace be = boost::endian;

class AuthResponse final {
	State state_ = State::INITIAL;

public:
	Result result;
	be::little_uint32_t queue_position = 0;
	be::little_uint32_t billing_time = 0;
	be::little_uint8_t billing_flags = 0;
	be::little_uint32_t billing_rested = 0;

	State read_from_stream(spark::BinaryInStream& stream) try {
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
	} catch(const spark::exception&) {
		return State::ERRORED;
	}

	void write_to_stream(spark::BinaryOutStream& stream) const {
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
