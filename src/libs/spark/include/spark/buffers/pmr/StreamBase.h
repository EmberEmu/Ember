/*
 * Copyright (c) 2021 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/pmr/BufferBase.h>
#include <spark/buffers/Shared.h>

namespace ember::spark::io::pmr {

class StreamBase {
	BufferBase& buffer_;
	StreamState state_;

protected:
	void set_state(StreamState state) {
		state_ = state;
	}

public:
	explicit StreamBase(BufferBase& buffer)
		: buffer_(buffer),
		  state_(StreamState::OK) { }

	std::size_t size() const {
		return buffer_.size();
	}

	[[nodiscard]]
	bool empty() const {
		return buffer_.empty();
	}

	StreamState state() const {
		return state_;
	}

	bool good() const {
		return state() == StreamState::OK;
	}

	void clear_state() {
		set_state(StreamState::OK);
	}

	operator bool() const {
		return good();
	}

	void set_error_state() {
		set_state(StreamState::USER_DEFINED_ERR);
	}

	virtual ~StreamBase() = default;
};

} // pmr, io, spark, ember
