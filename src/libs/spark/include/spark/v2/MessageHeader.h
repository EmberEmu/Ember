/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/endian/conversion.hpp>
#include <cstdint>

namespace ember::spark::v2 {

namespace be = boost::endian;

class MessageHeader final {
	constexpr static auto HEADER_SIZE = 6u;

	std::uint8_t alignment_ = 0;

public:
	enum class State {
		INITIAL, ERRORED, OK
	} state = State::INITIAL;

	std::uint32_t size = 0;
	std::uint8_t channel = 0;
	std::uint8_t padding = 0;

	template<typename reader>
	State read_from_stream(reader& stream) try {
		stream >> size;
		stream >> channel;
		stream >> padding;

		be::little_to_native_inplace(size);
		be::little_to_native_inplace(channel);
		be::little_to_native_inplace(padding);

		stream.skip(padding);
		return (state = State::OK);
	} catch(const std::exception&) {
		state = State::ERRORED;
		return state;
	}

	template<typename writer>
	void write_to_stream(writer& stream) const {
		auto write_size = HEADER_SIZE;
		auto pad = alignment_ - (write_size % alignment_);

		stream << be::native_to_little(size) + write_size + pad;
		stream << be::native_to_little(channel);
		stream << static_cast<std::uint8_t>(be::native_to_little(pad));

		// pad the header so the body starts at the correct alignment
		// the underlying buffer also needs to be properly aligned
		for(auto i = 0u; i < pad; ++i) {
			stream << std::uint8_t(0);
		}
	}

	void set_alignment(std::uint8_t alignment) {
		alignment_ = alignment;
	}
};

} // v2, spark, ember