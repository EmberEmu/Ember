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
#include <cstdint>
#include <cstddef>

namespace ember::grunt::client {

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

		/* 
		 * The game sends the compressed length, not the decompressed length,
		 * so we need some guesswork with the maximum buffer size for decompression.
		 * However, the client limits itself to 1000 bytes, which can cause the survey
		 * to fail with machines that have a healthy number of peripherals attached.
		 */
		std::vector<std::uint8_t> compressed(compressed_size_);
		stream.get(&compressed[0], compressed.size());
		data.resize(MAX_SURVEY_LEN);

		uLongf dest_len = data.size();

		auto ret = uncompress(reinterpret_cast<Bytef*>(data.data()), &dest_len, compressed.data(), compressed.size());

		if(ret != Z_OK) {
			throw bad_packet("Decompression of survey data failed with code " + std::to_string(ret));
		}
		
		data.resize(dest_len);
		state_ = State::DONE;
	}

public:
	SurveyResult() : Packet(Opcode::CMD_SURVEY_RESULT) {}

	std::uint32_t survey_id = 0;
	std::uint8_t error = 0;
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
		if(data.size() > MAX_SURVEY_LEN) {
			throw bad_packet("Survey data is too big");
		}

		stream << opcode;
		stream << be::native_to_little(survey_id);
		stream << error;

		std::vector<std::uint8_t> compressed(data.size());
		uLongf dest_len = compressed.size();

		auto ret = compress(compressed.data(), &dest_len, reinterpret_cast<const Bytef*>(data.data()), data.size());

		if(ret != Z_OK) {
			throw bad_packet("Compression of survey data failed with code " + std::to_string(ret));
		}

		stream << be::native_to_little(static_cast<std::uint16_t>(dest_len));
		stream.put(compressed.data(), dest_len);
	}
};

} // client, grunt, ember