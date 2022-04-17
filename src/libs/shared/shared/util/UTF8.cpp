/*
 * Copyright (c) 2016 - 2022 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "UTF8.h"
#include <utf8cpp/utf8.h>

namespace ember::util::utf8 {

// Operates on codepoints
std::size_t max_consecutive(const utf8_string& string) {
	const auto data_beg = string.data();
	const auto data_end = string.data() + string.size();
	auto beg = ::utf8::iterator(data_beg, data_beg, data_end);
	auto end = ::utf8::iterator(data_end, data_beg, data_end);
	auto it = beg;

	std::size_t current_run = 0;
	std::size_t longest_run = 0;
	std::uint32_t last = 0;

	while (it != end) {
		std::uint32_t current = *it;
		
		if(current == last && it != beg) {
			++current_run;
		} else {
			current_run = 0;
		}

		if(current_run > longest_run) {
			longest_run = current_run;
		}

		last = current;
	}

	return longest_run;
}

std::size_t length(const utf8_string& string) {
	return ::utf8::distance(string.begin(), string.end());
}

bool is_valid(const utf8_string& string) {
	return ::utf8::is_valid(string.begin(), string.end());
}

bool is_valid(const char* string, const std::size_t byte_length) {
	return ::utf8::is_valid(string, string + byte_length);
}

} // utf8, util, ember