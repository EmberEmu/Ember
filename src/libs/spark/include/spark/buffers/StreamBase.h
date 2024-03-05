/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BufferBase.h>
#include <type_traits>

namespace ember::spark {

template<typename T>
concept is_pod = std::is_standard_layout<T>::value && std::is_trivial<T>::value;

enum class StreamState {
	OK, READ_LIMIT_ERR, BUFF_LIMIT_ERR
};

class StreamBase {
	BufferBase& buffer_;

public:
	explicit StreamBase(BufferBase& buffer) : buffer_(buffer) { }


	std::size_t size() const {
		return buffer_.size();
	}

	bool empty() {
		return buffer_.empty();
	}

	virtual ~StreamBase() = default;
};

} // spark, ember
