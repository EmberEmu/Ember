/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <commands/Command.h>
#include <commands/Exception.h>
#include <commands/TypeMap.h>
#include <algorithm>
#include <ranges>

namespace ember::commands {

Command::Command(std::string name)
	: name_(std::move(name)) {
	if(name_.empty()) {
		throw exception("Command name cannot be empty");
	}
}

std::shared_ptr<Command> Command::create(std::string name) {
	return std::shared_ptr<Command>(new Command(std::move(name))); // make_shared can't access ctor
}

const std::type_info& Command::arg_type(const ArgumentValue& v) const {
	return std::visit([](auto&&x)->decltype(auto){
		return typeid(x); 
	}, v);
}

auto Command::validate_arg_count(const std::size_t count) const -> Result {
	if(count < required_arg_count()) {
		return Result::missing_args;
	} else if(count > args_.size()) {
		return Result::too_many_args;
	}

	return Result::success;
}

ArgumentStore Command::build_argument_store(std::span<const ArgumentValue> values) const {
	ArgumentStore arg_store;

	for(auto [value, arg] : std::views::zip(values, args_)) {
		if(!validate_type(arg.type, value)) {
			throw invalid_type(arg.name);
		}

		arg_store.emplace(arg.name, value);
	}

	return arg_store;
}

Result Command::execute() {
	return execute({});
}

Result Command::execute(std::span<const ArgumentValue> arg_values) {
	std::shared_ptr<CommandHandler> handler;
	ArgumentStore arg_store;

	{
		std::lock_guard guard(mutex_);

		if(auto validation = validate_arg_count(arg_values.size()); validation != Result::success) {
			return validation;
		}

		try {
			arg_store = std::move(build_argument_store(arg_values));
		} catch(invalid_type&) {
			return Result::invalid_types;
		}

		if(auto result = can_execute_handler(); result != Result::success) {
			return result;
		}
		
		handler = handler_;
	}

	// handler copy is used to prevent potential deadlocks if a handler invokes its own command
	// recursive mutex would still deadlock if the handler spawned a thread that subsequently called
	// the command again while the handler waited
	const Arguments arguments(std::move(arg_store));
	(*handler)(arguments);
	return Result::success;
}

auto Command::can_execute_handler() const -> Result {
	if(handler_) {
		return Result::success;
	} else {
		return commands_.empty()? Result::unavailable : Result::subcommands;
	}
}

std::shared_ptr<Command> Command::argument(std::string argument, ArgumentType type) {
	std::lock_guard guard(mutex_);

	if(optional_arg_count() > 0) {
		throw std::invalid_argument("Required arguments must be placed before optional arguments");
	}
		
	args_.emplace_back(std::move(argument), true, type);
	return this->shared_from_this();
}

std::shared_ptr<Command> Command::optional_argument(std::string argument, ArgumentType type) {
	std::lock_guard guard(mutex_);

	args_.emplace_back(std::move(argument), false, type);
	return this->shared_from_this();
}

std::shared_ptr<Command> Command::description(std::string description) {
	std::lock_guard guard(mutex_);

	description_ = std::move(description);
	return this->shared_from_this();
}

std::shared_ptr<Command> Command::handler(CommandHandler handler) {
	std::lock_guard guard(mutex_);

	// shared_ptr to avoid having to copy a potentially heavyweight handler
	// during invocation for thread safety reasons - guaranteeing the lifetime
	// of the handler rather than copying is sufficient
	handler_ = std::make_shared<CommandHandler>(handler);
	return this->shared_from_this();
}

std::vector<Argument> Command::arguments() const {
	std::lock_guard guard(mutex_);
	return args_;
}

const std::string& Command::name() const {
	return name_;
}

const std::string& Command::description() const {
	return description_;
}

std::size_t Command::required_arg_count() const {
	return std::ranges::count_if(args_, 
		[](const auto& arg){ return arg.required; });
}

std::size_t Command::optional_arg_count() const {
	return std::ranges::count_if(args_, 
		[](const auto& arg){ return !arg.required; });
}

CommandMap Command::commands() const {
	std::lock_guard guard(mutex_);
	return commands_;
}

std::string Command::usage_string() const {
	std::lock_guard guard(mutex_);

	if(args_.empty()) {
		return "<no arguments>";
	}

	std::string result;

	for(const auto& arg : args_) {
		result += (arg.required? " <" : " [") + arg.name + (arg.required ? ">" : "]");
	}

	return result;
}

std::shared_ptr<Command> Command::insert(std::string name) {
	std::lock_guard guard(mutex_);

	auto [entry, _] = commands_.insert_or_assign(
		name, create(name)
	);

	return entry->second;
}

ScopedCommand Command::scoped_insert(std::shared_ptr<Command> command) {
	insert(command);
	return ScopedCommand(command, this->weak_from_this());
}

void Command::insert(std::shared_ptr<Command> command) {
	std::lock_guard guard(mutex_);
	const auto& name = command->name(); // avoid issues if right-to-left evaluation
	commands_.insert_or_assign(name, std::move(command));
}

bool Command::erase_argument(const std::string& argument) {
	std::lock_guard guard(mutex_);

	const auto& remove = std::ranges::remove_if(args_, [&](auto& arg){
		return argument == arg.name;
	});

	args_.erase(remove.begin(), remove.end());
	return !!remove.size();
}

void Command::clear_arguments() {
	std::lock_guard guard(mutex_);
	args_.clear();
}

auto Command::erase(const std::string& name) -> std::optional<std::shared_ptr<Command>> {
	std::lock_guard guard(mutex_);

	auto result = commands_.extract(name);
	
	if(result.empty()) {
		return std::nullopt;
	}

	return result.mapped();
}

bool Command::erase(const std::shared_ptr<const Command>& command) {
	std::lock_guard guard(mutex_);

	if(auto it = commands_.find(command->name()); it != commands_.end()) {
		if(it->second == command) {
			commands_.erase(it);
			return true;
		}
	}
	
	return false;
}

void Command::clear_commands() {
	std::lock_guard guard(mutex_);
	commands_.clear();
}

bool Command::validate_type(ArgumentType type, const ArgumentValue& value) const {
	if(auto it = detail::types.find(type); it != detail::types.end()) {
		auto& [_, typeinfo] = *it;
		return typeinfo == arg_type(value);
	}

	return false;
}

std::size_t Command::argument_count() const {
	return args_.size();
}

Command& Command::operator()(std::shared_ptr<Command> command) {
	this->insert(command);
	return *this;
}

Command& Command::operator()(CommandHandler handler) {
	this->handler(handler);
	return *this;
}

Command& Command::operator()(std::string description) {
	this->description(description);
	return *this;
}

Command& Command::operator()(std::string argument, ArgumentType type, required) {
	this->argument(std::move(argument), type);
	return *this;
}

Command& Command::operator()(std::string argument, ArgumentType type, optional) {
	this->optional_argument(std::move(argument), type);
	return *this;
}

} // commands, ember