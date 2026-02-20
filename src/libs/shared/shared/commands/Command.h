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
#include "ArgumentType.h"
#include "Result.h"
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstddef>

namespace ember::commands {

class Command;

using CommandHandler = std::function<void(const Arguments&)>;
using CommandMap = std::unordered_map<std::string, std::shared_ptr<Command>>;

class Command : public std::enable_shared_from_this<Command> {
	mutable std::mutex mutex_;

	std::string description_;
	std::string name_;
	std::vector<Argument> args_;
	std::shared_ptr<CommandHandler> handler_;
	CommandMap subcommands_;

	Result validate_arg_count(std::size_t count) const;
	bool validate_types(const ArgumentStore& args) const;
	bool validate_type(ArgumentType type, const ArgumentValue& value) const;
	ArgumentStore build_argument_store(std::span<const ArgumentValue> values) const;
	std::size_t required_arg_count() const;
	std::size_t optional_arg_count() const;

	Command(std::string name);

public:
	static std::shared_ptr<Command> create(std::string name);

	Command(Command&) = delete;
	Command(Command&&) = delete;
	Command& operator=(Command&) = delete;
	Command& operator=(Command&&) = delete;

	void subcommand(std::shared_ptr<Command> command);
	std::shared_ptr<Command> subcommand(std::string name);
	std::shared_ptr<Command> description(std::string description);
	std::shared_ptr<Command> argument(std::string argument, ArgumentType type);
	std::shared_ptr<Command> optional_argument(std::string argument, ArgumentType type);
	std::shared_ptr<Command> handler(CommandHandler handler);
	void update_name(std::string name);
	bool remove_argument(const std::string& argument);
	void clear_arguments();
	std::optional<std::shared_ptr<Command>> remove_subcommand(const std::string& name);
	void clear_subcommands();

	const std::string& name() const;
	const std::string& description() const;
	std::vector<Argument> arguments() const;
	std::string usage_string() const;
	CommandMap subcommands() const;
	std::size_t argument_count() const;

	Result execute(std::span<const ArgumentValue> arguments);
};

} // commands, ember