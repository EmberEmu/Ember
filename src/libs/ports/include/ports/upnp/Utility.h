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

/*
  This exists because string_view isn't guaranteed to be null-terminated,
  (and we know ours isn't) so we can't use the standard atoi functions
*/ 
static int sv_to_int(const std::string_view string) {
	int value = 0;
	auto length = string.length();
	auto data = string.data();

	while(length) {
		value = (value * 10) + (*data - '0');
		++data;
		--length;
	}

	return value;
}

} // upnp, ports, ember