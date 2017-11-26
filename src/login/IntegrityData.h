/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include "grunt/Magic.h"
#include "ExecutablesChecksum.h"
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <cstdint>
#include <cstddef>

namespace ember {

class IntegrityData {
	std::unordered_map<std::size_t, std::vector<char>> data_;

	std::size_t hash(std::uint16_t build, grunt::Platform platform, grunt::System os) const;

	void load_binaries(const std::string& path, std::uint16_t build,
	                   const std::initializer_list<std::string_view>& files,
	                   grunt::System system, grunt::Platform platform);

public:
	IntegrityData(const std::vector<GameVersion>& versions, const std::string& path);

	std::optional<const std::vector<char>*> lookup(GameVersion version, grunt::Platform platform,
	                                               grunt::System os) const;
};


} // ember