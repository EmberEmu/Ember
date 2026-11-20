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
#include <commands/CommandsView.h>
#include <commands/Flags.h>
#include <commands/Result.h>
#include <commands/ParseArgument.h>
#include <commands/SearchResult.h>
#include <commands/ScopedCommand.h>
#include <commands/Suggestions.h>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>
#include <cstddef>

namespace ember::commands {

class Command;

using Handler = std::function<void(const Arguments&)>;
using Autocomplete = std::function<Suggestions(std::span<const std::string>)>;

class Command final : public std::enable_shared_from_this<Command> {
	struct _constructor_tag {
		explicit _constructor_tag() = default;
	};

	mutable std::mutex map_mutex_;
	mutable std::mutex mutex_;

	std::string description_;
	std::string name_;
	std::vector<Argument> args_;
	std::shared_ptr<Handler> handler_;
	Autocomplete autocomplete_;
	CommandMap commands_;
	Flags flags_;

	Result validate_arg_count(std::size_t count) const;
	ArgMap build_argument_map(std::span<const std::string> values) const;
	std::size_t required_arg_count() const;
	std::size_t optional_arg_count() const;
	Result can_execute_handler() const;
	Result execute_path(std::span<const std::string> arguments, bool require_match);

	std::shared_ptr<Command> insert_argument(std::string argument,
	                                         std::unique_ptr<ParseArgumentBase> parser,
	                                         const std::type_info& type,
	                                         bool required);

public:
	explicit Command(std::string name, _constructor_tag);

	static std::shared_ptr<Command> create(std::string name);

	Command(Command&) = delete;
	Command(Command&&) = delete;
	Command& operator=(Command&) = delete;
	Command& operator=(Command&&) = delete;

	std::shared_ptr<Command> description(std::string description);
	std::shared_ptr<Command> handler(Handler&& handler);
	std::shared_ptr<Command> flags(const Flags& flags);
	std::shared_ptr<Command> autocomplete(Autocomplete&& handler);
	ScopedCommand scoped_insert(std::string name);
	ScopedCommand scoped_insert(std::shared_ptr<Command> command);

	template<typename _ty>
	std::shared_ptr<Command> argument(std::string argument, required_t = required);

	template<typename _ty>
	std::shared_ptr<Command> argument(std::string argument, optional_t);

	bool erase_argument(const std::string& argument);
	void clear_arguments();
	std::optional<Suggestions> autocomplete(std::span<const std::string> tokens) const;

	const std::string& name() const;
	const std::string& description() const;
	std::vector<Argument> arguments() const;
	std::string usage_string() const;
	std::size_t argument_count() const;
	const Flags& flags() const;
	bool has_autocomplete() const;

	/**
	 * @brief Returns a locked view of the current command map
	 *
	 * While the view is held, you must not call any functions
	 * that interact with the map from the same thread
	 */
	CommandsView view() const;

	/**
	 * @brief Inserts a command
	 * @pre The calling thread must not hold a CommandsView
	 */
	void insert(std::shared_ptr<Command> command);

	/**
	 * @brief Inserts a command
	 * @pre The calling thread must not hold a CommandsView
	 */
	std::shared_ptr<Command> insert(std::string name);

	/**
	 * @brief Erases a command
	 * @pre The calling thread must not hold a CommandsView
	 */
	std::shared_ptr<Command> erase(const std::string_view name);

	/**
	 * @brief Erases a command
	 * @pre The calling thread must not hold a CommandsView
	 */
	bool erase(const std::shared_ptr<const Command>& command);

	/**
	 * @brief Clears all commands
	 * @pre The calling thread must not hold a CommandsView
	 */
	void clear_commands();

	Suggestions autocomplete(const std::string_view query) const;
	SearchResult find(const std::string_view query) const;
	SearchResult find(std::span<const std::string> tokens) const;

	Result execute();
	Result execute(std::span<const std::string> arguments);
	Result execute_path(std::span<const std::string> tokens);
	Result execute_path(std::string_view input);
};

} // commands, ember

#include <commands/Command.inl>