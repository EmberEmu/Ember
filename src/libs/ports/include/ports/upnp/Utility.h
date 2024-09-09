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
#include <span>
#include <string_view>
#include <type_traits>

namespace ember::ports::upnp {

struct CaseInsensitive {
	struct Comparator {
		bool operator() (std::string_view lhs, std::string_view rhs) const {
			return std::ranges::equal(lhs, rhs,
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
int sv_to_int(std::string_view string);
long sv_to_long(std::string_view string);
long long sv_to_ll(std::string_view string);

long long span_to_ll(std::span<const char> span);

/*
   Just a quick and dirty func. to extract values from HTTP fields (e.g. "max-age=300")
   C++ developers arguing about how best to split strings and on why
   the standard still provides no functionality for it will never not be funny
 */
std::string_view split_argument(std::string_view input, const char needle);

} // upnp, ports, ember