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
		SUCCESS                    = 0x00,
		NETWORK_FAILURE            = 0x01,
		SOAP_ARGUMENTS_MISMATCH    = 0x02,
		SOAP_MISSING_URI           = 0x03,
		SOAP_NO_ARGUMENTS          = 0x04,
		INVALID_MAPPING_ARG        = 0x05,
		OPERATION_ABORTED          = 0x06,
		HTTP_BAD_RESPONSE          = 0x80,
		HTTP_BAD_HEADERS           = 0x81,
		HTTP_NOT_OK                = 0x82,
		HTTP_HEADER_FIELD_AWOL     = 0x83,
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

} // upnp, ports, ember