/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <type_traits>
#include <cctype>

namespace ember::ports::upnp {

struct CaseInsensitive {
	struct Comparator {
		bool operator() (const std::string_view& lhs, const std::string_view& rhs) const {
			return std::equal(lhs.begin(), lhs.end(), rhs.begin(),
				[](char lhs, char rhs) {
					return std::tolower(lhs) == std::tolower(rhs);
				}
			);
		}
	};

	struct Hash {
		std::size_t operator() (std::string_view key) const {
			std::string hash_key(key.begin(), key.end());
			std::transform(hash_key.begin(), hash_key.end(), hash_key.begin(), ::tolower);
			return std::hash<std::string_view>{}(hash_key);
		}
	};
};

} // upnp, ports, ember