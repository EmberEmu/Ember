/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <string_view>
#include <cstddef>

namespace ember::utility {

inline std::string pretty_truncate(const std::string_view string, const std::size_t length) {
	if(string.size() <= length) {
		return std::string(string);
	}

	constexpr std::string_view ellipsis { "..." };

	if(length <= ellipsis.size()) {
		return std::string(ellipsis.substr(0, length));
	}

	const auto ellipsis_space = length - ellipsis.size();
	return std::string(string.substr(0, ellipsis_space)) + "...";
}

template<typename Stream>
inline void write_pretty_truncate(Stream& stream, const std::string_view string, const std::size_t length) {
	if(string.size() <= length) {
		stream << string;
	} else {
		stream << utility::pretty_truncate(string, length);
	}
}

} // utility, ember