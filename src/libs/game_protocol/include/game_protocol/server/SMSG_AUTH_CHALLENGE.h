/*
 * Copyright (c) 2015 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/endian/arithmetic.hpp>
#include <cstdint>
#include <cstddef>

namespace ember::protocol::smsg {

namespace be = boost::endian;

class SMSG_AUTH_CHALLENGE final {
	State state_ = State::INITIAL;

public:
	be::little_uint32_at seed;

	State read_from_stream(spark::BinaryStream& stream) {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		stream >> seed;

		return state_;
	}

	void write_to_stream(spark::BinaryStream& stream) const {
		stream << seed;
	}
};

} // protocol, ember
