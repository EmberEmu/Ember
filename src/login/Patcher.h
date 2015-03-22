/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include <string>
#include <vector>

namespace ember {

class Patcher {
	const std::string path_;
	const std::vector<GameVersion> versions_;

public:
	enum class PatchLevel { OK, TOO_OLD, PATCH_AVAILABLE, TOO_NEW };

	Patcher(std::vector<GameVersion> versions, std::string patch_path)
	        : versions_(std::move(versions)), path_(patch_path) {}

	PatchLevel check_version(const GameVersion& client_version) const;
};

} // ember