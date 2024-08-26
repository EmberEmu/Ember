/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <cstdint>
#include <shared/smartenum.hpp>

// These are to allow us to map both PCP & NATPMP replies onto
// a single structure - they are not protocol definitions
namespace ember::ports {

smart_enum_class(ErrorCode, int,
	SUCCESS,
	SERVER_INCOMPATIBLE,
	RESOLVE_FAILURE,
	CONNECTION_FAILURE,
	ID_MISMATCH,
	INTERNAL_ERROR,
	NO_RESPONSE,
	BAD_RESPONSE,
	WRONG_SOURCE,
	RETRY_NATPMP,
	PCP_CODE,
	NATPMP_CODE,
	INVALID_PROTOCOL
);

struct Error {
	Error(ErrorCode code)
		: code(code), pcp_code{} {}
	Error(ErrorCode code, natpmp::Result pmpcode)
		: code(code), natpmp_code(pmpcode) {}
	Error(ErrorCode code, pcp::Result pcpcode)
		: code(code), pcp_code(pcpcode) {}
	ErrorCode code;

	union {
		natpmp::Result natpmp_code;
		pcp::Result pcp_code;
	};

	friend bool operator==(const Error& lhs, const ErrorCode& rhs) {
		return lhs.code == rhs;
	}
};

} // ports, ember

CREATE_FORMATTER(ember::ports::ErrorCode)