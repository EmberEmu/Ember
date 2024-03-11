/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/upnp/HTTPTypes.h>
#include <ports/upnp/Utility.h>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ember::ports::upnp {

enum class ParseState {
	RESPONSE_CODE, HEADERS
};

// If there are any exploits in this project, it'll likely be in this code ":D"
static bool parse_http_header(const std::string_view text, HTTPHeader& header) {
	auto state = ParseState::RESPONSE_CODE;
	auto beg = text.begin();

	while(beg != text.end()) {
		auto it = std::find(beg, text.end(), '\n');

		if(it == text.end()) {
			break;
		}
		
		std::string_view line(beg, it);

		// HTTP spec says to expect \r\n but should be prepared to handle \n and ignore \r
		if(line.ends_with('\r')) {
			line.remove_suffix(1);
		}

		if(line.starts_with('\n')) {
			line.remove_prefix(1);
		}

		if(state == ParseState::RESPONSE_CODE) {
			constexpr auto expected_matches = 4;
			std::regex pattern(R"((\bHTTP\/1.1\b) ([0-9]{3}) ([a-zA-Z ]*))", std::regex::ECMAScript);
			std::match_results<typename decltype(line)::iterator> results;

			std::regex_search(line.begin(), line.end(), results, pattern);

			if(results.size() != expected_matches) {
				return false;
			}

			try {
				auto code = sv_to_int(line.substr(results.position(2), results.length(2)));
				header.code = HTTPResponseCode{ code };
			} catch(std::invalid_argument&) {
				return false;
			}

			header.text = line.substr(results.position(3), results.length(3));
			state = ParseState::HEADERS;

		} else if(state == ParseState::HEADERS) {
			auto pos = line.find_first_of(": ");

			if(pos == std::string_view::npos) {
				beg = ++it;
				continue;
			}

			header.fields.try_emplace(line.substr(0, pos),
				line.substr(pos + 2, line.size()));
		}

		if(*it == '\r') {
			++it;
		}

		if(*it == '\n') {
			++it;
		}

		beg = it;
	}

	return true;
}

} // upnp, ports, ember