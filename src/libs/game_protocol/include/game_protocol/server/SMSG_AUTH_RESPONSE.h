/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Packet.h>
#include <game_protocol/ResultCodes.h>
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstddef>

namespace ember { namespace protocol {

namespace be = boost::endian;

class SMSG_AUTH_RESPONSE final : public Packet {

	State state_ = State::INITIAL;

public:
	ResultCode result;
	std::uint32_t queue_position;

	State read_from_stream(spark::SafeBinaryStream& stream) override try {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		stream >> result;

		if(result == ResultCode::AUTH_WAIT_QUEUE) {
			stream >> queue_position;
		}

		return (state_ = State::DONE);
	} catch(spark::buffer_underrun&) {
		return State::ERRORED;
	}

	void write_to_stream(spark::SafeBinaryStream& stream) const override {
		stream << be::native_to_little(result);

		if(result == ResultCode::AUTH_WAIT_QUEUE) {
			stream << queue_position;
		}

		if(result == ResultCode::AUTH_OK) {
			stream << std::uint32_t(0);	// billing time remaining
			stream << std::uint8_t(0);	// billing plan flags
			stream << std::uint32_t(0);	// billing time rested
		}
	}

	std::uint16_t size() const override {
		std::uint16_t size = sizeof(result);

		if(result == ResultCode::AUTH_WAIT_QUEUE) {
			size += sizeof(queue_position);
		}

		if(result == ResultCode::AUTH_OK) {
			size += 9; // billing time stuff, hacky
		}

		return size;
	}
};

}} // protocol, ember