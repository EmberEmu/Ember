/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/Argument.h>
#include <commands/Command.h>
#include <commands/Exception.h>
#include <commands/Result.h>
#include <commands/Suggestions.h>
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
		std::size_t depth = 0;
	};

private:
	std::shared_ptr<Command> root_;

	static std::string longest_prefix(std::span<const Suggestions::Record> matches);

	Suggestions autocomplete_recurse(const CommandMap& commands, std::span<const std::string> tokens) const;

public:
	Registry();

	static std::vector<std::string> parse_input(const std::string_view input);

	std::shared_ptr<Command> insert(std::string name);
	void insert(std::shared_ptr<Command> command);
	std::optional<std::shared_ptr<Command>> erase(const std::string& name);
	bool erase(const std::shared_ptr<const Command> command);

	Suggestions autocomplete(const std::string_view query) const;

	SearchResult search(const std::string_view query) const;
	SearchResult search(std::span<const std::string> tokens) const;
	std::shared_ptr<Command> root() const;
};

} // commands, ember