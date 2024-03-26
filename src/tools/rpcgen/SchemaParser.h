/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Printer.h"
#include <filesystem>
#include <span>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace ember {

class SchemaParser {
	void verify(std::span<const std::uint8_t> buffer);
	std::vector<std::uint8_t> load_file(const std::filesystem::path& path);

public:
	void generate(const std::filesystem::path& path);
	void generate(std::span<const std::uint8_t> buffer);
};

} // ember