/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Argument.h"
#include "Result.h"
#include "Command.h"
#include "Suggestions.h"
#include <exception>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <cstddef>

namespace ember::commands {

class Registry {
public:
	struct SearchResult {
		std::shared_ptr<Command> command;
		std::size_t depth;
	};

private:
	Command root_;

	Suggestions autocomplete_recurse(const CommandMap& commands, std::span<const std::string> tokens) const;
	std::string longest_prefix(std::span<const Suggestions::Record> matches) const;

public:
	Registry();

	std::shared_ptr<Command> register_command(std::string name);
	void register_command(std::shared_ptr<Command> command);

	std::vector<std::string> parse_input(std::string_view input) const;
	Suggestions autocomplete(std::string_view query) const;

	SearchResult search(std::span<const std::string> tokens);
	bool unregister(const std::string& name);
};

} // commands, ember