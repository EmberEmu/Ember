/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stun/Attributes.h>
#include <stun/LogSeverity.h>
#include <shared/smartenum.hpp>
#include <functional>

namespace ember::stun {

/*
 * Opting to provide the client (ourselves) with a reason enumeration rather than
 * a string to make localising easier, if it ever gets that far.
 */
smart_enum_class(Error, std::uint8_t,
	ok,                               // s'all good, man
	bad_callback,
	no_response_received,             
	connection_aborted,               
	connection_reset,
	connection_error,
	unable_to_connect,
	bad_attribute_data,
	udp_test_only,
	unsupported_by_server,            // a test wasn't supported by the server
	buffer_parse_error,               // buffer stream reported an error, probably a bad attribute
	resp_buffer_lt_header,            // buffer was smaller than the fixed header length
	resp_ipv6_not_valid,              // received an ipv6 flag in RFC3489 mode (IPv4 only)
	resp_addr_fam_not_valid,          // address family was not valid (not IPv4 or IPv6)
	resp_cookie_missing,              // magic cookie value was incorrect in RFC5389 mode
	resp_bad_header_length,           // header specified attribute length as shorter than attribute header length
	resp_tx_not_found,                // transaction id was not found in the mapping, could be a delayed response
	resp_rfc5389_invalid_attribute,   // received attribute that isn't valid for RC5389
	resp_rfc3489_invalid_attribute,   // received attribute that isn't valid for RC3489
	resp_unknown_opt_attribute,       // received optional attribute that we couldn't parse
	resp_unknown_req_attribute,       // received required attribute that we couldn't parse
	resp_bad_req_attr_server,         // received required attribute that it shouldn't have
	resp_unk_attr_bad_pad,            // received UNKNOWN-ATTRIBUTES that wasn't a multiple of 4 bytes
	resp_error_string_bad_pad,        // received error reason that wasn't a multiple of 4 bytes
	resp_error_code_out_of_range,     // received error code that was out of range
	resp_bad_hmac_sha_attr,           // received a bad SHA HMAC attribute
	resp_bad_software_attr,           // received a bad software attribute
	resp_unexpected_attr,             // received an unexpected attribute
	resp_unhandled_resp_type,         // received an unhandled response type
	resp_missing_attr,                // response was missing an expected attribute
	resp_binding_error,               // server responded with an error 
	resp_bad_redirect,                // server tried to redirect us to a server we've already tried
	resp_unk_message_type,            // unknown or unhandled message type
	resp_invalid_fingerprint          // CRC32 in the fingerprint didn't match our own calculation
);

struct ErrorRet {
	explicit ErrorRet(Error reason, attributes::ErrorCode ec = {})
		: reason(reason), ec(std::move(ec)) {}
	Error reason;
	attributes::ErrorCode ec;
};

using LogCB = std::function<void(Severity, Error)>;

} // stun, ember