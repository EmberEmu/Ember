/*
* Copyright (c) 2024 - 2025 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <array>
#include <algorithm>
#include <bit>
#include <concepts>
#include <type_traits>
#include <cstddef>
#include <cstdint>

namespace ember::spark::io {

struct is_contiguous {};
struct is_non_contiguous {};
struct supported {};
struct unsupported {};
struct except_tag{};
struct allow_throw : except_tag{};
struct no_throw : except_tag{};

template<typename T> struct raw { T& str; };
template<typename T> struct raw_null_terminated { T& str; };
template<typename T> struct prefixed { T& str; };
template<typename T> struct prefixed_varint { T& str; };
template<typename T> struct null_terminated { T& str; };

enum class BufferSeek {
	SK_ABSOLUTE, SK_BACKWARD, SK_FORWARD
};

enum class StreamSeek {
	// Seeks within the entire underlying buffer
	SK_BUFFER_ABSOLUTE,
	SK_BACKWARD,
	SK_FORWARD,
	// Seeks only within the range written by the current stream
	SK_STREAM_ABSOLUTE
};

enum class StreamState {
	OK,
	READ_LIMIT_ERR,
	BUFF_LIMIT_ERR,
	INVALID_STREAM,
	USER_DEFINED_ERR
};

namespace detail {

template<typename size_type, typename stream_type>
constexpr auto varint_decode(stream_type& stream) -> std::pair<bool, size_type> {
	int shift { 0 };
	size_type value { 0 };
	std::uint8_t byte { 0 };

	do {
		// if reading another byte would violate the read limit
		if(stream.read_max() == 0) {
			return { false, 0 };
		}

		stream.get(&byte, 1);
		value |= (static_cast<size_type>(byte & 0x7F) << shift);
		shift += 7;
	} while(byte & 0x80);

	return { true, value };
}

template<typename size_type, typename stream_type>
constexpr auto varint_encode(stream_type& stream, size_type value) -> stream_type::size_type {
	typename stream_type::size_type written = 0;

	while(value > 0x7F) {
		const std::uint8_t byte = (value & 0x7F) | 0x80;
		stream.put(&byte, 1);
		value >>= 7;
		++written;
	}

	const std::uint8_t byte = value & 0x7F;
	stream.put(&byte, 1);
	return ++written;
}

template<decltype(auto) size>
constexpr auto generate_filled(const std::uint8_t value) {
	std::array<std::uint8_t, size> target{};
	std::ranges::fill(target, value);
	return target;
}

// Returns true if there's any overlap between source and destination ranges
static inline bool region_overlap(const void* src, std::size_t src_len, const void* dst, std::size_t dst_len) {
	const auto src_beg = std::bit_cast<std::uintptr_t>(src);
	const auto src_end = src_beg + src_len;
	const auto dst_beg = std::bit_cast<std::uintptr_t>(dst);
	const auto dst_end = dst_beg + dst_len;

	// cannot assume src is before dst or vice versa
	return (src_beg >= dst_beg && src_beg < dst_end)
		|| (src_end > dst_beg && src_end <= dst_end)
		|| (dst_beg >= src_beg && dst_beg < src_end)
		|| (dst_end > src_beg && dst_end <= src_end);
}

} // detail

} // io, spark, ember