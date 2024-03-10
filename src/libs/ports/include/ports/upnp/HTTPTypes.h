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

enum class HTTPResponseCode {
	HTTP_OK = 200
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
	HTTPResponseCode code;
	std::string_view text;
	std::unordered_map<std::string_view, std::string_view,
		CaseInsensitive::Hash, CaseInsensitive::Comparator> fields;
};

} // upnp, ports, ember