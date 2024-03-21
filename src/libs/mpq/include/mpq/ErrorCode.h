/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ostream>

namespace ember::mpq {

struct ErrorCode {
	enum {
		SUCCESS,
		NO_ARCHIVE_FOUND,
		BAD_ALIGNMENT,
		FILE_NOT_FOUND,
		UNABLE_TO_OPEN,
		FILE_READ_FAILED
	} val_;

	ErrorCode(decltype(val_) value) : val_(value) {}

	explicit operator bool() {
		return val_ != ErrorCode::SUCCESS;
	}

	friend std::ostream& operator<< (std::ostream& os, const ErrorCode& ec)
	{
		return os << ec.val_;
	}

	constexpr int value() const {
		return val_;
	}
};

inline bool operator==(const ErrorCode& lhs, const ErrorCode& rhs) {
	return lhs.value() == rhs.value();
}

inline bool operator!=(const ErrorCode& lhs, const ErrorCode& rhs) {
	return !(lhs == rhs);
}

} // mpq, ember