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
#include <mutex>
#include <optional>
#include <unordered_map>
#include <sstream>
#include <string>
#include <vector>
#include <cstddef>

namespace ember::commands {

class Registry {
public:
	enum class Result {
		Success,
		NotFound,
		TooManyArgs,
		MissingArgs,
		BadInput,
		NoHandler
	};

private:
	std::unordered_map<std::string, Command> registry_;
	mutable std::mutex lock_;

	Result validate_arg_count(std::size_t count, const Command& cmd) const;

public:
	std::vector<std::string> parse_input(const std::string& input) const;
	std::vector<std::string> autocomplete(std::string& cmd_buffer) const;

	Result execute_command(const std::string& input);
	Result execute_command(const std::vector<std::string>& input);
	Command& register_command(std::string name);
	std::optional<Command> get_command(std::string name);
};

} // commands, ember