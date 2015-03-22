/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Patcher.h"
#include <algorithm>

namespace ember {

auto Patcher::check_version(const GameVersion& client_version) const -> PatchLevel {
	if(std::find(versions_.begin(), versions_.end(), client_version) != versions_.end()) {
		return PatchLevel::OK;
	}

	// Figure out whether any of the allowed client versions are newer than the client.
	// If so, there's a chance that it can be patched.
	for(auto& v : versions_) {
		if(v > client_version) {
			return PatchLevel::TOO_OLD;
		}
	}

	return PatchLevel::TOO_NEW;
}

} // ember