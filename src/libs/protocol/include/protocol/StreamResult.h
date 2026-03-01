/*
 * Copyright (c) 2025 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <ostream>
#include <string_view>
#include <cassert>

namespace ember::protocol {

struct StreamResult {
	enum {
		success              = 0x00,
		failed               = 0x01,
		caught_exception     = 0x02,
		bad_field_size       = 0x03,
		decompression_failed = 0x04,
		compression_failed   = 0x05,
		too_big              = 0x06,
		stream_error         = 0x07,

		stream_result_max    = stream_error + 1
	} val_;

	std::array<std::string_view, stream_result_max> result_strings {
		"success",
		"unspecified failure",
		"encountered an exception",
		"bad data size",
		"data size too large",
		"decompression failed",
		"compression failed",
		"underlying stream error"
	};

	StreamResult(decltype(val_) value) : val_(value) {}

	explicit operator bool() {
		return val_ == StreamResult::success;
	}

	friend std::ostream& operator<< (std::ostream& os, const StreamResult& ec) {
		return os << ec.val_;
	}

	constexpr int value() const {
		return val_;
	}

	std::string_view what() const {
		assert(val_ < result_strings.size());
		return result_strings[val_];
	}
};

inline bool operator==(const StreamResult& lhs, const StreamResult& rhs) {
	return lhs.value() == rhs.value();
}

inline bool operator!=(const StreamResult& lhs, const StreamResult& rhs) {
	return !(lhs == rhs);
}

} // protocol, ember