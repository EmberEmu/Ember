/*
 * Copyright (c) 2016 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Packet.h>
#include <stdexcept>
#include <cstdint>
#include <cstddef>

namespace ember::protocol::client {

class CharacterEnum final {
	State state_ = State::INITIAL;

public:
	State read_from_stream(auto& stream) try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");
		return (state_ = State::DONE);
	} catch(const std::exception&) {
		return State::ERRORED;
	}

	void write_to_stream(auto& stream) const {
	}
};

} // protocol, ember
