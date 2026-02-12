/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Argument.h"
#include "Arguments.h"
#include <functional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace ember::commands {

using CommandHandler = std::function<void(const Arguments&)>;

class Command {
	int req_args = 1;
	int opt_args = 0;
	int total_args = 1;
	std::string description_;
	std::string name_;
	std::vector<Argument> args_;
	CommandHandler handler_;

	std::unordered_map<std::string, Command> subcommands_;

public:
	Command(std::string name);

	Command& description(std::string description);
	Command& arg(std::string argument);
	Command& optional_arg(std::string argument);
	Command& handler(CommandHandler handler);
	Command& subcommand(std::string name);
	std::span<const Argument> args() const;
	std::string usage_string() const;

	friend class Registry;
};

} // commands, ember