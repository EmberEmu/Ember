/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/smartenum.hpp>
#include <functional>

namespace ember::stun {

/*
 * STUN_LOG prefix is just to reduce the odds of annoying define collisions from
 * rage-inducing headers such as Windows.h when they end up being accidentally
 * transitively included through ten layers of crap.
 */
enum class Verbosity {
	STUN_LOG_TRIVIAL,
	STUN_LOG_DEBUG,
	STUN_LOG_INFO,
	STUN_LOG_WARN,
	STUN_LOG_ERROR,
	STUN_LOG_FATAL
};

/*
 * Opting to provide the client (ourselves) with a reason enumeration rather than
 * a string to make localising easier, if it ever gets that far.
 */
smart_enum_class(Error, std::uint8_t,
	OK,                               // s'all good, man
	BAD_CALLBACK,
	NO_RESPONSE_RECEIVED,             
	CONNECTION_ABORTED,               
	CONNECTION_RESET,
	CONNECTION_ERROR,
	UNABLE_TO_CONNECT,
	BAD_ATTRIBUTE_DATA,
	UDP_TEST_ONLY,
	UNSUPPORTED_BY_SERVER,            // a test wasn't supported by the server
	BUFFER_PARSE_ERROR,               // buffer stream reported an error, probably a bad attribute
	RESP_BUFFER_LT_HEADER,            // buffer was smaller than the fixed header length
	RESP_IPV6_NOT_VALID,              // received an IPv6 flag in RFC3489 mode (IPv4 only)
	RESP_ADDR_FAM_NOT_VALID,          // address family was not valid (not IPv4 or IPv6)
	RESP_COOKIE_MISSING,              // magic cookie value was incorrect in RFC5389 mode
	RESP_BAD_HEADER_LENGTH,           // header specified attribute length as shorter than attribute header length
	RESP_TX_NOT_FOUND,                // transaction ID was not found in the mapping, could be a delayed response
	RESP_RFC5389_INVALID_ATTRIBUTE,   // received attribute that isn't valid for RFC5389
	RESP_RFC3489_INVALID_ATTRIBUTE,   // received attribute that isn't valid for RFC3489
	RESP_UNKNOWN_OPT_ATTRIBUTE,       // received optional attribute that we couldn't parse
	RESP_UNKNOWN_REQ_ATTRIBUTE,       // received required attribute that we couldn't parse
	RESP_BAD_REQ_ATTR_SERVER,         // received required attribute that it shouldn't have
	RESP_UNK_ATTR_BAD_PAD,            // received UNKNOWN-ATTRIBUTES that wasn't a multiple of 4 bytes
	RESP_ERROR_STRING_BAD_PAD,        // received error reason that wasn't a multiple of 4 bytes
	RESP_ERROR_CODE_OUT_OF_RANGE,     // received error code that was out of range
	RESP_BAD_HMAC_SHA_ATTR,           // received a bad SHA HMAC attribute
	RESP_BAD_SOFTWARE_ATTR,           // received a bad software attribute
	RESP_UNEXPECTED_ATTR,             // received an unexpected attribute
	RESP_UNHANDLED_RESP_TYPE,         // received an unhandled response type
	RESP_MISSING_ATTR,                // response was missing an expected attribute
	RESP_BINDING_ERROR,               // server responded with an error 
	RESP_BAD_REDIRECT,                // server tried to redirect us to a server we've already tried
	RESP_UNK_MESSAGE_TYPE,            // unknown or unhandled message type
	RESP_INVALID_FINGERPRINT          // crc32 in the fingerprint didn't match our own calculation
);

struct ErrorRet {
	explicit ErrorRet(Error reason, attributes::ErrorCode ec = {})
		: reason(reason), ec(std::move(ec)) {}
	Error reason;
	attributes::ErrorCode ec;
};

using LogCB = std::function<void(Verbosity, Error)>;

} // stun, ember