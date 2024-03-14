/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/upnp/Utility.h>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ember::ports::upnp {

enum class HTTPStatus {
	CONTINUE = 100,
	SWITCHING_PROTOCOLS = 101,
	PROCESSING = 102,
	EARLY_HINTS = 103,
	OK = 200,
	CREATED = 201,
	ACCEPTED = 202,
	NON_AUTHORITATIVE = 203,
	NO_CONTENT = 204,
	RESET_CONTENT = 205,
	PARTIAL_CONTENT = 206,
	MULTI_STATUS = 207,
	ALREADY_REPORTED = 208,
	IM_USED = 226,
	MULTIPLE_CHOICES = 300,
	MOVED_PERMANENTLY = 301,
	FOUND = 302,
	SEE_OTHER = 303,
	NOT_MODIFIED = 304,
	USE_PROXY = 305,
	SWITCH_PROXY = 306,
	TEMPORARY_REDIRECT = 307,
	PERMANENT_REDIRECT = 308,
	BAD_REQUEST = 400,
	UNAUTHORISED = 401,
	PAYMENT_REQUIRED = 402,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
	METHOD_NOT_AVAILABLE = 405,
	NOT_ACCEPTABLE = 406,
	PROXY_AUTH_REQUIRED = 407,
	REQUEST_TIMEOUT = 408,
	CONFLICT = 409,
	GONE = 410,
	LENGTH_REQUIRED = 411,
	PRECONDITION_FAILED = 412,
	PAYLOAD_TOO_LARGE = 413,
	URI_TOO_LONG = 414,
	UNSUPPORTED_MEDIA_TYPE = 415,
	RANGE_NOT_SATISFIABLE = 416,
	EXPECTATION_FAILED = 417,
	IM_A_TEAPOT = 418,
	MISDIRECTED_REQUEST = 421,
	UNPROCESSABLE_CONTENT = 422,
	LOCKED = 423,
	FAILED_DEPENDENCY = 424,
	TOO_EARLY = 425,
	UPGRADE_REQUIRED = 426,
	PRECONDITION_REQUIRED = 428,
	TOO_MANY_REQUESTS = 429,
	REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
	UNAVAILABLE_FOR_LEGAL_REASONS = 451,
	INTERNAL_SERVER_ERROR = 500,
	NOT_IMPLEMENTED = 501,
	BAD_GATEWAY = 502,
	SERVICE_UNAVAILABLE = 503,
	GATEWAY_TIMEOUT = 504,
	HTTP_VERSION_NOT_SUPPORTED = 505,
	VARIANT_ALSO_NEGOTIATES = 506,
	INSUFFICIENT_STORAGE = 507,
	LOOP_DETECTED = 508,
	NOT_EXTENDED = 510,
	NETWORK_AUTHENTICATION_REQUIRED = 511
};

struct HTTPRequest {
	std::string method;
	std::string url;
	std::vector<std::pair<std::string, std::string>> fields;
	std::string body;
};

/*
  This is non-owning, so the buffer used to produce it must
  be kept in scope for at least as long as any instances
*/
struct HTTPHeader {
	using Field = std::pair<std::string_view, std::string_view>;
	HTTPStatus code{};
	std::string_view text;
	std::unordered_map<std::string_view, std::string_view,
		CaseInsensitive::Hash, CaseInsensitive::Comparator> fields;
};

} // upnp, ports, ember