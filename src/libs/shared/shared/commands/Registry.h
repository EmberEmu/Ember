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
#include <exception>
#include <functional>
#include <optional>
#include <unordered_map>
#include <sstream>
#include <string>
#include <vector>

namespace ember::commands {

class Registry {
	std::unordered_map<std::string, Command> registry_;

public:
	enum class Result {
		Success,
		NotFound,
		TooManyArgs,
		MissingArgs,
		BadInput
	};

	std::vector<std::string> parse_input(const std::string& input) const;

	Result execute_command(const std::string& input);
	Result execute_command(const std::vector<std::string>& input);
	Command& register_command(std::string name);
	std::optional<Command> get_command(std::string name);
	std::vector<std::string> autocomplete(std::string& cmd);
};

} // commands, ember