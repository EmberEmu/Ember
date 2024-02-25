/*
 * Copyright (c) 2015 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Packet.h>
#include <boost/assert.hpp>
#include <boost/endian/arithmetic.hpp>
#include <stdexcept>
#include <cstdint>
#include <cstddef>

namespace ember::protocol::server {

namespace be = boost::endian;

class AuthChallenge final {
	State state_ = State::INITIAL;

public:
	be::little_uint32_t seed;

	template<typename reader>
	State read_from_stream(reader& stream) try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		stream >> seed;
		return (state_ = State::DONE);
	} catch(const std::exception&) {
		state_ = State::ERRORED;
	}

	template<typename writer>
	void write_to_stream(writer& stream) const {
		stream << seed;
	}
};

} // protocol, ember
