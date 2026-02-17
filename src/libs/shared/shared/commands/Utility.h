/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <span>
#include <string>
#include <cstddef>

namespace ember::commands {

constexpr auto approx_cmd_len = 10u;

inline std::string path_fragment(std::span<const std::string> tokens, std::size_t depth) {
	if(tokens.size() < depth) {
		return {};
	}

	std::string fragment;
	fragment.reserve(depth * approx_cmd_len);

	for(auto i = 0; i < depth; ++i) {
		fragment += tokens[i];

		if(i != depth - 1) {
			fragment += " ";
		}
	}

	return fragment;
}

} // commands, ember