/*
* Copyright (c) 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <type_traits>
#include <cstdint>
#include <cstddef>

namespace ember::spark::io {

template<typename buf_t>
concept byte_oriented = sizeof(typename buf_t::value_type) == 1;

template<typename type>
concept byte_type = sizeof(type) == 1;

template<typename T>
concept is_pod = std::is_standard_layout<T>::value && std::is_trivial<T>::value;

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
	OK, READ_LIMIT_ERR, BUFF_LIMIT_ERR
};

struct is_contiguous {};
struct is_non_contiguous {};
struct supported {};
struct unsupported {};

// Returns true if there's any overlap between source and destination ranges
template<typename Src, typename Dst>
static inline bool region_overlap(const Src* src, std::size_t src_len, const Dst* dst, std::size_t dst_len) {
	const auto src_beg = reinterpret_cast<std::uintptr_t>(src);
	const auto src_end = src_beg + src_len;
	const auto dst_beg = reinterpret_cast<std::uintptr_t>(dst);
	const auto dst_end = dst_beg + dst_len;

	// cannot assume src is before dst or vice versa
	return (src_beg >= dst_beg && src_beg < dst_end)
		|| (src_end > dst_beg && src_end <= dst_end)
		|| (dst_beg >= src_beg && dst_beg < src_end)
		|| (dst_end > src_beg && dst_end <= src_end);
}

} // io, spark, ember