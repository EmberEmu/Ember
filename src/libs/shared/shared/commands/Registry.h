/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Argument.h"
#include "Command.h"
#include "Exception.h"
#include "Result.h"
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
		std::size_t depth = 0;
	};

private:
	std::shared_ptr<Command> root_;

	Suggestions autocomplete_recurse(const CommandMap& commands, std::span<const std::string> tokens) const;
	static std::string longest_prefix(std::span<const Suggestions::Record> matches);

public:
	Registry();

	std::shared_ptr<Command> register_command(std::string name);
	void register_command(std::shared_ptr<Command> command);

	static std::vector<std::string> parse_input(std::string_view input);
	Suggestions autocomplete(std::string_view query) const;

	SearchResult search(std::span<const std::string> tokens) const;
	bool unregister(const std::string& name);
	std::shared_ptr<Command> root() const;
};

} // commands, ember