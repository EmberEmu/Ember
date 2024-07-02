/*
* Copyright (c) 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <type_traits>

namespace ember::spark {

template<typename buf_t>
concept byte_oriented = sizeof(typename buf_t::value_type) == 1;

template<typename T>
concept is_pod = std::is_standard_layout<T>::value && std::is_trivial<T>::value;

enum class BufferSeek {
	SK_ABSOLUTE, SK_BACKWARD, SK_FORWARD
};

enum class StreamSeek {
	SK_BUFFER_ABSOLUTE, SK_BACKWARD, SK_FORWARD, SK_STREAM_ABSOLUTE
};

enum class StreamState {
	OK, READ_LIMIT_ERR, BUFF_LIMIT_ERR
};

} // spark, ember