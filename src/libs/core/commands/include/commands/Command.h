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
#include <commands/CommandMap.h>
#include <commands/Flags.h>
#include <commands/Result.h>
#include <commands/SearchResult.h>
#include <commands/ScopedCommand.h>
#include <commands/Suggestions.h>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <typeinfo>
#include <vector>
#include <cstddef>

namespace ember::commands {

class Command;

using Handler = std::function<void(const Arguments&)>;

class Command : public std::enable_shared_from_this<Command> {
	struct _constructor_tag {
		explicit _constructor_tag() = default;
	};

	mutable std::mutex mutex_;

	std::string description_;
	std::string name_;
	std::vector<Argument> args_;
	std::shared_ptr<Handler> handler_;
	CommandMap commands_;
	Flags flags_;

	Result validate_arg_count(std::size_t count) const;
	bool validate_type(const std::type_info& type, const std::any& value) const;
	ArgMap build_argument_map(std::span<std::any> values) const;
	std::size_t required_arg_count() const;
	std::size_t optional_arg_count() const;
	Result can_execute_handler() const;
	std::shared_ptr<Command> insert_argument(std::string argument, const std::type_info& type);
	std::shared_ptr<Command> insert_optional_argument(std::string argument, const std::type_info& type);

public:
	explicit Command(std::string name, _constructor_tag);

	static std::shared_ptr<Command> create(std::string name);

	Command(Command&) = delete;
	Command(Command&&) = delete;
	Command& operator=(Command&) = delete;
	Command& operator=(Command&&) = delete;

	void insert(std::shared_ptr<Command> command);
	std::shared_ptr<Command> insert(std::string name);
	std::shared_ptr<Command> description(std::string description);
	std::shared_ptr<Command> handler(Handler handler);
	std::shared_ptr<Command> flags(const Flags& flags);
	ScopedCommand scoped_insert(std::string name);
	ScopedCommand scoped_insert(std::shared_ptr<Command> command);

	template<typename _ty>
	std::shared_ptr<Command> argument(std::string argument, required_t = required) {
		return insert_argument(std::move(argument), typeid(_ty));
	}

	template<typename _ty>
	std::shared_ptr<Command> argument(std::string argument, optional_t) {
		return insert_optional_argument(std::move(argument), typeid(_ty));
	}

	bool erase_argument(const std::string& argument);
	void clear_arguments();
	std::shared_ptr<Command> erase(const std::string_view name);
	bool erase(const std::shared_ptr<const Command>& command);
	void clear_commands();

	const std::string& name() const;
	const std::string& description() const;
	std::vector<Argument> arguments() const;
	std::string usage_string() const;
	CommandMap commands() const;
	std::size_t argument_count() const;
	const Flags& flags() const;

	Suggestions autocomplete(const std::string_view query) const;
	SearchResult find(const std::string_view query) const;
	SearchResult find(std::span<const std::string> tokens) const;

	Result execute();
	Result execute(std::span<std::any> arg_values);
};

} // commands, ember