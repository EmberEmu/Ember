/*
* Copyright (c) 2024 - 2026 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include <string_view>
#include <cstddef>

namespace ember::commands::impl {

struct StringHash {
	using hash_type = std::hash<std::string_view>;
	using is_transparent = void;

	std::size_t operator()(const char* str) const {
		return hash_type{}(str);
	}

	std::size_t operator()(const std::string_view str) const {
		return hash_type{}(str);
	}

	std::size_t operator()(const std::string& str) const {
		return hash_type{}(str);
	}
};

} // impl, commands, ember
