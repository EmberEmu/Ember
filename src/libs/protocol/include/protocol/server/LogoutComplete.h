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
#include <spark/buffers/BinaryStream.h>
#include <boost/assert.hpp>
#include <cstdint>
#include <cstddef>

namespace ember::protocol::server {

class LogoutComplete final {
	State state_ = State::INITIAL;

public:
	Result result;

	State read_from_stream(spark::BinaryStreamReader& stream) try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		stream >> result;

		return (state_ = State::DONE);
	} catch(const spark::exception&) {
		return State::ERRORED;
	}

	void write_to_stream(spark::BinaryStreamWriter& stream) const {
		stream << result;
	}
};

} // protocol, ember
