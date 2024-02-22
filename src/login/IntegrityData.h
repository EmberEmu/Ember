/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include "grunt/Magic.h"
#include "ExecutablesChecksum.h"
#include <optional>
#include <string>
#include <span>
#include <string_view>
#include <unordered_map>
#include <cstdint>
#include <cstddef>

namespace ember {

class IntegrityData {
	std::unordered_map<std::size_t, std::vector<std::byte>> data_;

	std::size_t hash(std::uint16_t build, grunt::Platform platform, grunt::System os) const;

	void load_binaries(std::string_view path, std::uint16_t build,
	                   std::span<std::string_view> files,
	                   grunt::System system, grunt::Platform platform);

public:
	IntegrityData(std::span<const GameVersion> versions, std::string_view path);

	std::optional<std::span<const std::byte>> lookup(GameVersion version,
	                                                 grunt::Platform platform,
	                                                 grunt::System os) const;
};


} // ember