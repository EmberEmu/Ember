/*
* Copyright (c) 2024 - 2025 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

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