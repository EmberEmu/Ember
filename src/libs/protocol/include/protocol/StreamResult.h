/*
 * Copyright (c) 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <ostream>
#include <string_view>

namespace ember::protocol {

struct StreamResult {
	enum {
		SUCCESS              = 0x00,
		FAILED               = 0x01,
		BAD_FIELD_SIZE       = 0x02,
		DECOMPRESSION_FAILED = 0x03,
		COMPRESSION_FAILED   = 0x04,
		TOO_BIG              = 0x05,
		STREAM_ERROR         = 0x06,

		STREAM_RESULT_MAX    = STREAM_ERROR + 1
	} val_;

	std::array<std::string_view, STREAM_RESULT_MAX> result_strings {
		"success",
		"unspecified failure",
		"bad data size",
		"data size too large",
		"decompression failed",
		"compression failed",
		"underlying stream error"
	};

	StreamResult(decltype(val_) value) : val_(value) {}

	explicit operator bool() {
		return val_ == StreamResult::SUCCESS;
	}

	friend std::ostream& operator<< (std::ostream& os, const StreamResult& ec) {
		return os << ec.val_;
	}

	constexpr int value() const {
		return val_;
	}

	std::string_view what() const {
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