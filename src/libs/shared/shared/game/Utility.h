/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include <gsl/narrow>
#include <regex>
#include <istream>

namespace ember {

inline std::istream& operator>>(std::istream& in, GameVersion& version) try {
	constexpr auto expected_matches = 5;

	std::string build_string;
	std::string line;

	while(std::getline(in, line)) {
		build_string += line;
	}

	in.clear();

	std::regex pattern(
		R"(^\s*([+-]?\d+)\s*[,.]\s*([+-]?\d+)\s*[,.]\s*([+-]?\d+)\s*[,.]\s*([+-]?\d+)\s*$)"
	);

	std::smatch matches;

	if(!std::regex_match(build_string, matches, pattern)) {
		in.setstate(std::ios_base::failbit);
		return in;
	}

	if(matches.size() != expected_matches) {
		in.setstate(std::ios_base::failbit);
	}

	version = GameVersion{
		.major = gsl::narrow<std::uint8_t>(std::stoi(matches[1])),
		.minor = gsl::narrow<std::uint8_t>(std::stoi(matches[2])),
		.patch = gsl::narrow<std::uint8_t>(std::stoi(matches[3])),
		.build = gsl::narrow<std::uint16_t>(std::stoi(matches[4])),
	};

	return in;
} catch(...) {
	in.setstate(std::ios_base::failbit);
	return in;
}

} // ember