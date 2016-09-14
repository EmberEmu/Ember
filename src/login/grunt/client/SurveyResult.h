/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Opcodes.h"
#include "../Packet.h"
#include "../Exceptions.h"
#include <boost/assert.hpp>
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstddef>

namespace ember { namespace grunt { namespace client {

namespace be = boost::endian;

class SurveyResult final : public Packet {
	static const std::size_t MIN_READ_LENGTH = 8;
	State state_ = State::INITIAL;

	void read_body(spark::SafeBinaryStream& stream) {
		stream >> opcode;
		stream >> survey_id;
		stream >> error;
		stream >> size;

		be::little_to_native_inplace(survey_id);
		be::little_to_native_inplace(size);
	}

	void read_data(spark::SafeBinaryStream& stream) {
		if(error) {
			state_ = State::DONE;
			return;
		}

		if(stream.size() >= size) {
			data.resize(size);
			stream.get(&data[0], data.size());
			state_ = State::DONE;
		} else {
			state_ = State::CALL_AGAIN;
		}
	}

public:
	SurveyResult() : Packet(Opcode::CMD_SURVEY_RESULT) {}

	std::uint32_t survey_id;
	std::uint8_t error;
	std::uint16_t size;
	std::vector<std::uint8_t> data;

	State read_from_stream(spark::SafeBinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < MIN_READ_LENGTH) {
			return State::CALL_AGAIN;
		}

		switch(state_) {
			case State::INITIAL:
				read_body(stream);
				[[fallthrough]];
			case State::CALL_AGAIN:
				read_data(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}
		
		return state_;
	}

	void write_to_stream(spark::BinaryStream& stream) const override {
		stream << opcode;
		stream << be::native_to_little(survey_id);
		stream << error;
		stream << be::native_to_little(std::uint16_t(data.size()));
		stream.put(data.data(), data.size());
	}
};

}}} // client, grunt, ember