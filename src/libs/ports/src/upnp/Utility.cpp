/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/Utility.h>
#include <stdexcept>
#include <cctype>

namespace ember::ports::upnp {

int span_to_int(std::span<const char> span) {
	int value = 0;
	auto length = span.size_bytes();
	auto data = span.begin();

	while(length) {
		if(std::isdigit(*data) == 0) {
			throw std::invalid_argument("span_to_int: cannot convert");
		}

		value = (value * 10) + (*data - '0');
		++data;
		--length;
	}

	return value;
}

/*
  This exists because string_view isn't guaranteed to be null-terminated,
  (and we know ours isn't) so we can't use the standard atoi functions
*/ 
int sv_to_int(const std::string_view string) {
	return span_to_int(string);
}

/*
   Just a quick and dirty func. to extract values from HTTP fields (e.g. "max-age=300")
   C++ developers arguing about how best to split strings and on why
   the standard still provides no functionality for it will never not be funny
 */
std::string_view split_argument(std::string_view input, const char needle) {
	const auto pos = input.find_last_of(needle);

	if(!pos) {
		throw std::invalid_argument("split_view, can't find needle");
	}

	if(pos == input.size() - 1) {
		throw std::invalid_argument("split_view, nothing after needle");
	}

	return input.substr(pos + 1, input.size());
}

} // upnp, ports, ember