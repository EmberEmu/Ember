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
#include <shared/util/UTF8String.h>
#include <boost/assert.hpp>
#include <boost/endian/arithmetic.hpp>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <cstddef>

namespace ember::protocol::server {

namespace be = boost::endian;

class CharacterRename final {
	State state_ = State::INITIAL;

public:
	Result result;
	be::little_uint64_t id;
	utf8_string name;
	
	template<typename reader>
	State read_from_stream(reader& stream) try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		stream >> result;

		if(result == protocol::Result::RESPONSE_SUCCESS) {
			stream >> id;
			stream >> name;
		}

		return (state_ = State::DONE);
	} catch(const std::exception&) {
		return State::ERRORED;
	}

	template<typename writer>
	void write_to_stream(writer& stream) const {
		stream << result;

		if(result == protocol::Result::RESPONSE_SUCCESS) {
			stream << id;
			stream << name;
		}
	}
};

} // protocol, ember
