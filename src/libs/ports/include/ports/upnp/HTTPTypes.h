/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/upnp/Utility.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ember::ports::upnp {

enum class HTTPStatus {
	continue_ = 100,
	switching_protocols = 101,
	processing = 102,
	early_hints = 103,
	ok = 200,
	created = 201,
	accepted = 202,
	non_authoritative = 203,
	no_content = 204,
	reset_content = 205,
	partial_content = 206,
	multi_status = 207,
	already_reported = 208,
	im_used = 226,
	multiple_choices = 300,
	moved_permanently = 301,
	found = 302,
	see_other = 303,
	not_modified = 304,
	use_proxy = 305,
	switch_proxy = 306,
	temporary_redirect = 307,
	permanent_redirect = 308,
	bad_request = 400,
	unauthorised = 401,
	payment_required = 402,
	forbidden = 403,
	not_found = 404,
	method_not_available = 405,
	not_acceptable = 406,
	proxy_auth_required = 407,
	request_timeout = 408,
	conflict = 409,
	gone = 410,
	length_required = 411,
	precondition_failed = 412,
	payload_too_large = 413,
	uri_too_long = 414,
	unsupported_media_type = 415,
	range_not_satisfiable = 416,
	expectation_failed = 417,
	im_a_teapot = 418,
	misdirected_request = 421,
	unprocessable_content = 422,
	locked = 423,
	failed_dependency = 424,
	too_early = 425,
	upgrade_required = 426,
	precondition_required = 428,
	too_many_requests = 429,
	request_header_fields_too_large = 431,
	unavailable_for_legal_reasons = 451,
	internal_server_error = 500,
	not_implemented = 501,
	bad_gateway = 502,
	service_unavailable = 503,
	gateway_timeout = 504,
	http_version_not_supported = 505,
	variant_also_negotiates = 506,
	insufficient_storage = 507,
	loop_detected = 508,
	not_extended = 510,
	network_authentication_required = 511
};

struct HTTPRequest {
	std::string_view method;
	std::string_view url;
	std::vector<std::pair<std::string_view, std::string_view>> fields;
	std::string_view body;
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