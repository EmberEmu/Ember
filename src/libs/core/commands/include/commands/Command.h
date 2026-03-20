/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/Argument.h>
#include <commands/Arguments.h>
#include <commands/ArgumentMap.h>
#include <commands/ArgumentValue.h>
#include <commands/ArgumentType.h>
#include <commands/Flags.h>
#include <commands/Result.h>
#include <commands/ScopedCommand.h>
#include <commands/detail/StringHash.h>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#include <cstddef>

namespace ember::commands {

class Command;

using CommandHandler = std::function<void(const Arguments&)>;
using CommandMap = std::unordered_map<std::string, std::shared_ptr<Command>, StringHash, std::equal_to<>>;

class Command : public std::enable_shared_from_this<Command> {
	mutable std::mutex mutex_;

	std::string description_;
	std::string name_;
	std::vector<Argument> args_;
	std::shared_ptr<CommandHandler> handler_;
	CommandMap commands_;
	Flags flags_;

	Result validate_arg_count(std::size_t count) const;
	bool validate_type(args::Type type, const args::Value& value) const;
	ArgMap build_argument_map(std::span<args::Value> values) const;
	std::size_t required_arg_count() const;
	std::size_t optional_arg_count() const;
	Result can_execute_handler() const;
	const std::type_info& arg_type(const args::Value& v) const;

	explicit Command(std::string name);

public:
	struct required {};
	struct optional {};

	static std::shared_ptr<Command> create(std::string name);

	Command(Command&) = delete;
	Command(Command&&) = delete;
	Command& operator=(Command&) = delete;
	Command& operator=(Command&&) = delete;

	std::shared_ptr<Command> insert(std::string name);
	std::shared_ptr<Command> description(std::string description);
	std::shared_ptr<Command> argument(std::string argument, args::Type type);
	std::shared_ptr<Command> optional_argument(std::string argument, args::Type type);
	std::shared_ptr<Command> handler(CommandHandler handler);
	std::shared_ptr<Command> flags(const Flags& flags);

	void insert(std::shared_ptr<Command> command);
	ScopedCommand scoped_insert(std::shared_ptr<Command> command);
	bool erase_argument(const std::string& argument);
	void clear_arguments();
	std::optional<std::shared_ptr<Command>> erase(const std::string_view name);
	bool erase(const std::shared_ptr<const Command>& command);
	void clear_commands();

	const std::string& name() const;
	const std::string& description() const;
	std::vector<Argument> arguments() const;
	std::string usage_string() const;
	CommandMap commands() const;
	std::size_t argument_count() const;
	const Flags& flags() const;

	Result execute();
	Result execute(std::span<args::Value> arg_values);

	Command& operator()(CommandHandler handler);
	Command& operator()(std::shared_ptr<Command> command);
	Command& operator()(std::string description);
	Command& operator()(const Flags& flags);
	Command& operator()(std::string argument, args::Type type, required);
	Command& operator()(std::string argument, args::Type type, optional);

	friend class PrefixedRegistry; 
};

} // commands, ember