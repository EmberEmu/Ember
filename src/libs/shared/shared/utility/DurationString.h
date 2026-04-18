/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <chrono>
#include <regex>
#include <string>

using namespace std::chrono_literals;

namespace ember::utility {

struct DurationString {
	std::chrono::seconds value;

	DurationString(const std::string& input)
		: value(parse(input)) {	}

	std::chrono::seconds parse(const std::string& input) {
		const std::regex pattern(R"((\d+d)?(\d+h)?(\d+m)?(\d+s)?)");
		std::smatch matches;

		if(!std::regex_match(input, matches, pattern)) {
			throw std::invalid_argument("invalid duration format");
		}

		auto duration = 0s;

		if(matches[1].matched) {
			const auto days = std::stoi(matches[1].str());
			duration += std::chrono::days(days);
		}

		if(matches[2].matched) {
			const auto hours = std::stoi(matches[2].str());
			duration += std::chrono::hours(hours);
		}

		if(matches[3].matched) {
			const auto minutes = std::stoi(matches[3].str());
			duration += std::chrono::minutes(minutes);
		}

		if(matches[4].matched) {
			const auto seconds = std::stoi(matches[4].str());
			duration += std::chrono::seconds(seconds);
		}

		return duration;
	}
};

} // utility, ember