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
#include "Result.h"
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace ember::commands {

class Command;

using CommandHandler = std::function<void(const Arguments&)>;
using CommandMap = std::unordered_map<std::string, std::shared_ptr<Command>>;

class Command {
	mutable std::mutex mutex_;

	std::string description_;
	std::string name_;
	std::vector<Argument> args_;
	std::shared_ptr<CommandHandler> handler_;
	CommandMap subcommands_;

	Result validate_arg_count(std::size_t count) const;
	ArgumentStore build_argument_store(std::span<const std::string> tokens) const;
	std::size_t required_arg_count() const;
	std::size_t optional_arg_count() const;

public:
	Command(std::string name);

	Command(Command&) = delete;
	Command(Command&&) = delete;
	Command& operator=(Command&) = delete;
	Command& operator=(Command&&) = delete;

	std::shared_ptr<Command> subcommand(std::string name);
	Command& description(std::string description);
	Command& arg(std::string argument);
	Command& optional_arg(std::string argument);
	Command& handler(CommandHandler handler);
	bool remove_arg(const std::string& argument);
	void clear_args();
	bool remove_subcommand(const std::string& name);
	void clear_subcommands();

	const std::string& name() const;
	const std::string& description() const;
	std::vector<Argument> args() const;
	std::string usage_string() const;
	CommandMap subcommands() const;

	Result execute(std::span<const std::string> arguments);

	Command* operator->() {
		return this;
	}
};

} // commands, ember