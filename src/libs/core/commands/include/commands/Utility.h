/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/CommandMap.h>
#include <commands/Exception.h>
#include <commands/Suggestions.h>
#include <span>
#include <string>
#include <string_view>
#include <cstddef>

namespace ember::commands {

namespace impl {

Suggestions autocomplete_recurse(const CommandMap& commands, std::span<const std::string> tokens);
std::string longest_prefix(std::span<const Suggestions::Record> matches);

} // impl

std::shared_ptr<Command> create(std::string name);
std::vector<std::string> parse_input(const std::string_view input, bool escape = true);
std::string path_fragment(std::span<const std::string> tokens, std::size_t depth);

} // commands, ember