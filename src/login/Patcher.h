/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include "PatchMeta.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace ember {

class Patcher {
	const std::vector<PatchMeta> patches_;
	const std::vector<GameVersion> versions_;
	FileMeta survey_;
	bool survey_active_;

	void generate_graph();

public:
	enum class PatchLevel { OK, TOO_OLD, PATCH_AVAILABLE, TOO_NEW };

	Patcher(std::vector<GameVersion> versions, std::vector<PatchMeta> patches);
	void set_survey(FileMeta survey);
	bool survey_active() const;
	boost::optional<FileMeta> find_patch(const GameVersion& client_version) const;
	PatchLevel check_version(const GameVersion& client_version) const;
};

} // ember