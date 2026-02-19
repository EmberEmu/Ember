/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ostream>

namespace ember::ports::upnp {

struct ErrorCode {
	enum {
		success                    = 0x00,
		network_failure            = 0x01,
		soap_arguments_mismatch    = 0x02,
		soap_missing_uri           = 0x03,
		soap_no_arguments          = 0x04,
		invalid_mapping_arg        = 0x05,
		operation_aborted          = 0x06,
		http_bad_response          = 0x80,
		http_bad_headers           = 0x81,
		http_not_ok                = 0x82,
		http_header_field_awol     = 0x83,
	} val_;

	ErrorCode(decltype(val_) value) : val_(value) {}

	explicit operator bool() {
		return val_ != ErrorCode::success;
	}

	friend std::ostream& operator<< (std::ostream& os, const ErrorCode& ec) {
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

} // upnp, ports, ember