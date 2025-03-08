/*
 * Copyright (c) 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ostream>

namespace ember::protocol {

struct StreamResult {
	enum {
		SUCCESS = 0x00,
		FAILED  = 0x01
	} val_;

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
};

inline bool operator==(const StreamResult& lhs, const StreamResult& rhs) {
	return lhs.value() == rhs.value();
}

inline bool operator!=(const StreamResult& lhs, const StreamResult& rhs) {
	return !(lhs == rhs);
}

} // protocol, ember