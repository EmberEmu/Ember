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
#include <string_view>

namespace ember::protogen {

struct ScopeEntry {
	std::string name;
	std::string type;
};

inline const ScopeEntry* lookup(std::span<const ScopeEntry> scope, std::string_view name) {
	for(auto it = scope.rbegin(); it != scope.rend(); ++it) {
		if(it->name == name) {
			return &*it;
		}
	}

	return nullptr;
}

} // protogen, ember
