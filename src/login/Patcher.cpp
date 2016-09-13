/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Patcher.h"
#include <algorithm>
#include <regex>
#include <cstdio>

namespace ember {

Patcher::Patcher(std::vector<GameVersion> versions, std::vector<PatchMeta> patches)
                 : versions_(std::move(versions)), patches_(std::move(patches)),
                   survey_active_(false) {
	generate_graph();
}

void Patcher::generate_graph() {

}

boost::optional<FileMeta> Patcher::find_patch(const GameVersion& client_version) const {
	return boost::optional<FileMeta>();
}

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

void Patcher::set_survey(FileMeta survey) {
	survey_ = std::move(survey);
	survey_active_ = true;
}

bool Patcher::survey_active() const {
	return survey_active_;
}

} // ember