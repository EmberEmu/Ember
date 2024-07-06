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

namespace ember::spark {

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

static inline bool region_overlap(const void* first1, const void* last1, const void* first2) {
	const auto f1 = reinterpret_cast<std::uintptr_t>(first1);
	const auto l1 = reinterpret_cast<std::uintptr_t>(last1);
	const auto f2 = reinterpret_cast<std::uintptr_t>(first2);
	return f2 >= f1 && f2 < l1;
}

} // spark, ember