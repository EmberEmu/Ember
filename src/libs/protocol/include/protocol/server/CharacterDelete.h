/*
 * Copyright (c) 2016 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Packet.h>
#include <protocol/ResultCodes.h>
#include <boost/assert.hpp>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember::protocol::server {

class CharacterDelete final {
	State state_ = State::INITIAL;

public:
	Result result;
	
	template<typename reader>
	State read_from_stream(reader& stream) try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		stream >> result;

		return (state_ = State::DONE);
	} catch(const std::exception&) {
		return State::ERRORED;
	}

	template<typename writer>
	void write_to_stream(writer& stream) const {
		stream << result;
	}
};

} // protocol, ember
