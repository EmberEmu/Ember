/*
 * Copyright (c) 2016 - 2026 Ember
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
#include <gsl/narrow>
#include <zlib.h>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::client {

class SurveyResult final : public Packet {
	static const std::size_t min_read_length = 8;
	static const std::size_t max_survey_len  = 8192;
	State state_ = State::initial;

	be::little_uint16_t compressed_size_ = 0;

	void read_body(PacketStream& stream) {
		stream >> opcode;
		stream >> survey_id;
		stream >> error;
		stream >> compressed_size_;
	}

	void read_data(PacketStream& stream) {
		if(error) {
			state_ = State::done;
			return;
		}

		if(stream.size() < compressed_size_) {
			state_ = State::call_again;
			return;
		}

		/* 
		 * The game sends the compressed length, not the decompressed length,
		 * so we need some guesswork with the maximum buffer size for decompression.
		 * However, the client limits itself to 1000 bytes, which can cause the survey
		 * to fail with machines that have a healthy number of peripherals attached.
		 */
		std::vector<std::uint8_t> compressed(compressed_size_);
		stream.get(compressed);

		int ret = Z_OK;

		data.resize_and_overwrite(max_survey_len, [&](char* strbuf, std::size_t size) {
			uLongf dest_len = size;
			ret = uncompress(reinterpret_cast<Bytef*>(strbuf), &dest_len, compressed.data(), compressed.size());
			return dest_len;
		});

		if(ret != Z_OK) {
			throw bad_packet("Decompression of survey data failed with code " + std::to_string(ret));
		}
		
		state_ = State::done;
	}

public:
	SurveyResult()
		: Packet(Opcode::cmd_survey_result) {}

	std::uint32_t survey_id = 0;
	std::uint8_t error = 0;
	std::string data;

	State read_from_stream(PacketStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(state_ == State::initial && stream.size() < min_read_length) {
			return State::call_again;
		}

		switch(state_) {
			case State::initial:
				read_body(stream);
				[[fallthrough]];
			case State::call_again:
				read_data(stream);
				break;
			default:
				BOOST_ASSERT_MSG(false, "Unreachable condition hit");
		}
		
		return state_;
	}

	void write_to_stream(PacketStream& stream) const override {
		if(data.size() > max_survey_len) {
			throw bad_packet("Survey data is too big");
		}

		stream << opcode;
		stream << survey_id;
		stream << error;

		std::vector<std::uint8_t> compressed(data.size());
		uLongf dest_len = compressed.size();

		auto ret = compress(compressed.data(), &dest_len, reinterpret_cast<const Bytef*>(data.data()), data.size());

		if(ret != Z_OK) {
			throw bad_packet("Compression of survey data failed with code " + std::to_string(ret));
		}

		stream << gsl::narrow<std::uint16_t>(dest_len);
		stream.put(compressed.data(), dest_len);
	}
};

} // client, grunt, ember