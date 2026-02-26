/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <type_traits>
#include <cstddef>
#include <cstdint>

namespace ember::spark::io {

struct is_contiguous {};
struct is_non_contiguous {};
struct except_tag {};
struct allow_throw_t final : except_tag {};
struct no_throw_t final : except_tag {};

[[maybe_unused]] constexpr static no_throw_t no_throw {};
[[maybe_unused]] constexpr static allow_throw_t allow_throw {};

struct init_empty_t {};
constexpr static init_empty_t init_empty {};

#define STRING_ADAPTOR(adaptor_name)                                      \
template<typename string_type, std::integral prefix_type = std::uint32_t> \
struct adaptor_name {                                                     \
    using size_type = prefix_type;                                        \
    string_type& str;                                                     \
    string_type* operator->() { return &str; }                            \
};                                                                        \
/* deduction guide required for clang 17 support */                       \
template<typename string_type>                                            \
adaptor_name(string_type&) -> adaptor_name<string_type>;                  \

STRING_ADAPTOR(raw)
STRING_ADAPTOR(prefixed)
STRING_ADAPTOR(prefixed_varint)
STRING_ADAPTOR(null_terminated)

enum class BufferSeek {
	sk_absolute,
	sk_backward,
	sk_forward
};

enum class StreamSeek {
	// Seeks within the entire underlying buffer
	sk_buffer_absolute,
	sk_backward,
	sk_forward,
	// Seeks only within the range written by the current stream
	sk_stream_absolute
};

enum class StreamState {
	ok,
	read_limit_error,
	buffer_limit_error,
	buffer_write_error,
	invalid_stream,
	user_defined_error
};

namespace detail {

template<typename size_type, typename stream_type>
constexpr auto varint_decode(stream_type& stream) -> size_type {
	int shift { 0 };
	size_type value { 0 };
	std::uint8_t byte { 0 };

	do {
		byte = 0; // clear in case an error occurs
		stream.get(&byte, 1);
		value |= (static_cast<size_type>(byte & 0x7f) << shift);
		shift += 7;
	} while(byte & 0x80);

	return value;
}

template<typename size_type, typename stream_type>
constexpr auto varint_encode(stream_type& stream, size_type value) -> size_type {
	size_type written = 0;

	while(value > 0x7f) {
		const std::uint8_t byte = (value & 0x7f) | 0x80;
		stream.put(&byte, 1);
		value >>= 7;
		++written;
	}

	const std::uint8_t byte = value & 0x7f;
	stream.put(&byte, 1);
	return ++written;
}

template<decltype(auto) size>
static constexpr auto generate_filled(const std::uint8_t value) {
	std::array<std::uint8_t, size> target{};
	std::ranges::fill(target, value);
	return target;
}

// Returns true if there's any overlap between source and destination ranges
[[maybe_unused]]
static inline bool region_overlap(const void* src, std::size_t src_len, const void* dst, std::size_t dst_len) {
	const auto src_beg = std::bit_cast<std::uintptr_t>(src);
	const auto src_end = src_beg + src_len;
	const auto dst_beg = std::bit_cast<std::uintptr_t>(dst);
	const auto dst_end = dst_beg + dst_len;
	return src_beg < dst_end && dst_beg < src_end;
}

} // detail

} // io, spark, ember
