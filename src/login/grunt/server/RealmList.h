/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Opcodes.h"
#include "../Packet.h"
#include "../ResultCodes.h"
#include <cstdint>
#include <cstddef>

namespace ember { namespace grunt { namespace server {

class RealmList : public Packet {
	static const std::size_t WIRE_LENGTH = 0;

	State state_ = State::INITIAL;

public:
	State read_from_stream(spark::BinaryStream& stream) {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		return (state_ = State::DONE);
	}

	void write_to_stream(spark::BinaryStream& stream) {

	}
};

}}} // server, grunt, ember