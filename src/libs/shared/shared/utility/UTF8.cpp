/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "UTF8.h"
#include <utf8cpp/utf8.h>
#include <boost/locale.hpp>
#include <cctype>
#include <locale>
#include <cstdint>
#include <cstddef>

namespace ember::utility::utf8 {


utf8_string name_format(const utf8_string& string) {
	if(string.empty()) {
		return {};
	}

	const auto wide = boost::locale::conv::utf_to_utf<wchar_t>(string);

	std::wstring formatted;
	formatted.reserve(wide.size());
	formatted += boost::locale::to_upper(std::wstring(1, wide.front()));

	if(wide.size() > 1) {
		formatted += boost::locale::to_lower(wide.substr(1));
	}

	return boost::locale::conv::utf_to_utf<char>(formatted);
}

bool is_alpha(const utf8_string& string) {
	const auto wide = boost::locale::conv::utf_to_utf<wchar_t>(string);

	for(const wchar_t codepoint : wide) {
		if(!std::isalpha(codepoint)) {
			return false;
		}
	}

	return true;
}

// Operates on codepoints
std::size_t max_consecutive(const utf8_string& string, const bool case_insensitive) {
	const auto folded = case_insensitive? boost::locale::to_lower(string) : string;

	const auto data_beg = folded.data();
	const auto data_end = data_beg + folded.size();

	auto it = ::utf8::iterator(data_beg, data_beg, data_end);
	auto end = ::utf8::iterator(data_end, data_beg, data_end);

	std::size_t current_run = 0;
	std::size_t longest_run = 0;
	char32_t last = 0;

	while(it != end) {
		const char32_t current = static_cast<char32_t>(*it);

		if(current == last) {
			++current_run;
		} else {
			current_run = 1;
		}

		longest_run = std::max(longest_run, current_run);
		last = current;
		++it;
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