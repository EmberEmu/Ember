/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "UTF8.h"
#include <utf8cpp/utf8.h>

namespace ember::util::utf8 {

std::size_t length(const std::string& utf8_string, bool& valid) try {
	valid = true;
	return ::utf8::distance(utf8_string.begin(), utf8_string.end());
} catch(::utf8::exception&) {
	valid = false;
	return 0;
}

bool is_valid(const std::string& utf8_string) {
	return ::utf8::is_valid(utf8_string.begin(), utf8_string.end());
}

bool is_valid(const char* utf8_string, std::size_t byte_length) {
	return ::utf8::is_valid(utf8_string, utf8_string + byte_length);
}

} // utf8, util, ember