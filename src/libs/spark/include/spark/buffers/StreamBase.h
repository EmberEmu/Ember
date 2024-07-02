/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/BufferBase.h>
#include <spark/buffers/Utility.h>

namespace ember::spark {

class StreamBase {
	BufferBase& buffer_;

public:
	explicit StreamBase(BufferBase& buffer) : buffer_(buffer) { }


	std::size_t size() const {
		return buffer_.size();
	}

	bool empty() const {
		return buffer_.empty();
	}

	virtual ~StreamBase() = default;
};

} // spark, ember
