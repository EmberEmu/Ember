/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "grunt/Magic.h"
#include <shared/database/objects/PatchMeta.h>
#include <span>
#include <string>
#include <optional>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace ember {

class Survey {
	FileMeta file_{};
	std::vector<std::byte> data_;
	std::uint32_t id_;

	void set(const std::string& path, std::uint32_t id);
public:
	Survey(std::uint32_t id = 0) : id_(id) {}

	std::uint32_t id() const;
	FileMeta meta(grunt::Platform platform, grunt::System os) const;
	void add_data(grunt::Platform platform, grunt::System os, const std::string& path);
	
	[[nodiscard]]
	std::optional<std::span<const std::byte>> data(grunt::Platform platform,
	                                               grunt::System os) const;
};

};