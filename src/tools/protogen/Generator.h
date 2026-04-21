/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Validator.h"
#include <jsoncons/json.hpp>
#include <filesystem>
#include <string>
#include <span>

namespace ember::protogen {

struct GeneratedFile {
	std::string relative_path;
	std::string content;
};

GeneratedFile generate_message(const jsoncons::json& message,
                               const TypeRegistry& registry,
                               const std::filesystem::path& templates_dir);

std::string generate_aggregator(std::span<const std::string> relative_paths,
                                const std::filesystem::path& templates_dir);

} // protogen, ember
