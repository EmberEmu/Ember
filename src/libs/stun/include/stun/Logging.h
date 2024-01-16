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
smart_enum_class(LogReason, std::uint8_t,
	RESP_BUFFER_LT_HEADER,            // buffer was smaller than the fixed header length
	RESP_IPV6_NOT_VALID,              // received an IPv6 flag in RFC3489 mode (IPv4 only)
	RESP_ADDR_FAM_NOT_VALID,          // address family was not valid (not IPv4 or IPv6)
	RESP_COOKIE_MISSING,              // magic cookie value was incorrect in RFC5389 mode
	RESP_BAD_HEADER_LENGTH,           // header specified attribute length as shorter than attribute header length
	RESP_TX_NOT_FOUND,                // transaction ID was not found in the mapping, could be a delayed response
	RESP_RFC5389_INVALID_ATTRIBUTE,   // encountered an attribute that isn't valid for RFC5389
	RESP_RFC3489_INVALID_ATTRIBUTE,   // encountered an attribute that isn't valid for RFC3489
	RESP_UNKNOWN_OPT_ATTRIBUTE,       // encountered an optional attribute that we couldn't parse
	RESP_UNKNOWN_REQ_ATTRIBUTE,       // encountered a required attribute that we couldn't parse
	RESP_BAD_REQ_ATTR_SERVER          // server sent us a required attribute that it shouldn't have
);

using LogCB = std::function<void(Verbosity, LogReason reason)>;

} // stun, ember