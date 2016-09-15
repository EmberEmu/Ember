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
#include <zlib.h>
#include <iostream>
#include <cstdint>
#include <cstddef>

namespace ember { namespace grunt { namespace client {

namespace be = boost::endian;

class SurveyResult final : public Packet {
	static const std::size_t MIN_READ_LENGTH = 8;
	static const std::size_t MAX_SURVEY_LEN  = 8192;
	State state_ = State::INITIAL;
	std::uint16_t compressed_size_ = 0;

	void read_body(spark::SafeBinaryStream& stream) {
		stream >> opcode;
		stream >> survey_id;
		stream >> error;
		stream >> compressed_size_;

		be::little_to_native_inplace(survey_id);
		be::little_to_native_inplace(compressed_size_);
	}

	void read_data(spark::SafeBinaryStream& stream) {
		if(error) {
			state_ = State::DONE;
			return;
		}

		if(stream.size() < compressed_size_) {
			state_ = State::CALL_AGAIN;
			return;
		}

		std::vector<std::uint8_t> compressed(compressed_size_);
		stream.get(&compressed[0], compressed.size());
		data.resize(MAX_SURVEY_LEN);

		uLongf dest_len = data.size();

		auto ret = uncompress(reinterpret_cast<Bytef*>(&data[0]), &dest_len, compressed.data(), compressed.size());

		if(ret != Z_OK) {
			throw bad_packet("Decompression of survey data failed with code " + ret);
		}
		
		std::cout << "Decompressed size: " << dest_len << " Packet reported: " << compressed_size_ << "\n";
		data.resize(dest_len);
		state_ = State::DONE;
	}

public:
	SurveyResult() : Packet(Opcode::CMD_SURVEY_RESULT) {}

	std::uint32_t survey_id;
	std::uint8_t error;
	std::string data;

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
		/*stream << be::native_to_little(std::uint16_t(data.size()));
		stream.put(data.data(), data.size());*/
	}
};

}}} // client, grunt, ember