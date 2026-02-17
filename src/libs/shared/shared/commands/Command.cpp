/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Command.h"
#include "Exception.h"
#include <algorithm>
#include <ranges>
#include <cassert>

namespace ember::commands {

Command::Command(std::string name)
	: name_(name) {
	if(name_.empty()) {
		throw exception("Command name cannot be empty");
	}
}

std::shared_ptr<Command> Command::create(std::string name) {
	return std::shared_ptr<Command>(new Command(name)); // make_shared can't access ctor
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
		ParsedArgument parsed(value, arg.type, arg.required);
		arg_store.emplace(arg.value, std::move(parsed));
	}

	return arg_store;
}

bool Command::validate_type(ArgumentType type, const ArgumentValue& value) const {
	switch(type) {
		case ArgumentType::at_string:
			if(std::holds_alternative<std::string>(value)) { return false; }
			break;
		case ArgumentType::at_uint8:
			if(std::holds_alternative<std::uint8_t>(value)) { return false; }
			break;
		case ArgumentType::at_uint16:
			if(std::holds_alternative<std::uint16_t>(value)) { return false; }
			break;
		case ArgumentType::at_uint32:
			if(std::holds_alternative<std::uint32_t>(value)) { return false; }
			break;
		case ArgumentType::at_uint64:
			if(std::holds_alternative<std::uint64_t>(value)) { return false; }
			break;
		case ArgumentType::at_int8:
			if(std::holds_alternative<std::int8_t>(value)) { return false; }
			break;
		case ArgumentType::at_int16:
			if(std::holds_alternative<std::int16_t>(value)) { return false; }
			break;
		case ArgumentType::at_int32:
			if(std::holds_alternative<std::int32_t>(value)) { return false; }
			break;
		case ArgumentType::at_int64:
			if(std::holds_alternative<std::int64_t>(value)) { return false; }
			break;
		case ArgumentType::at_float:
			if(std::holds_alternative<float>(value)) { return false; }
			break;
		case ArgumentType::at_double:
			if(std::holds_alternative<double>(value)) { return false; }
			break;
		case ArgumentType::at_char:
			if(std::holds_alternative<char>(value)) { return false; }
			break;
		default:
			return false;
	}

	return true;
}

bool Command::validate_types(const ArgumentStore& args) const {
	for(auto& [_, v] : args) {
		if(!validate_type(v.type, v.value)) {
			return false;
		}
	}

	return true;
}

Result Command::execute(std::span<const ArgumentValue> arguments) {
	std::shared_ptr<CommandHandler> handler;
	ArgumentStore arg_store;

	{
		std::lock_guard guard(mutex_);

		if(auto validation = validate_arg_count(arguments.size()); validation != Result::success) {
			return validation;
		}

		arg_store = std::move(build_argument_store(arguments));

		if(!validate_types(arg_store)) {
			return Result::invalid_types;
		}

		if(handler_) {
			handler = handler_;
		} else {
			return subcommands_.empty()? Result::unavailable : Result::subcommands;
		}
	}

	// handler copy is used to prevent potential deadlocks if a handler invokes its own command
	// recursive mutex would still deadlock if the handler spawned a thread that subsequently called
	// the command again while the handler waited
	(*handler)(std::move(arg_store));
	return Result::success;
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

std::vector<Argument> Command::args() const {
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

CommandMap Command::subcommands() const {
	std::lock_guard guard(mutex_);
	return subcommands_;
}

std::string Command::usage_string() const {
	std::lock_guard guard(mutex_);

	std::string result;

	if(args_.empty()) {
		return "<no arguments>";
	}

	for(const auto& arg : args_) {
		result += (arg.required ? " <" : " [") + arg.value + (arg.required ? ">" : "]");
	}

	return result;
}

std::shared_ptr<Command> Command::subcommand(std::string name) {
	std::lock_guard guard(mutex_);

	auto [entry, _] = subcommands_.insert_or_assign(
		name, create(name)
	);

	return entry->second;
}

void Command::subcommand(std::shared_ptr<Command> command) {
	std::lock_guard guard(mutex_);
	const auto& name = command->name(); // avoid issues if right-to-left evaluation
	subcommands_.insert_or_assign(name, std::move(command));
}

bool Command::remove_arg(const std::string& argument) {
	std::lock_guard guard(mutex_);

	auto remove = std::ranges::remove_if(args_, [&](auto& arg){
		return argument == arg.value;
	});

	args_.erase(remove.begin(), args_.end());
	return !!remove.size();
}

void Command::clear_args() {
	std::lock_guard guard(mutex_);
	args_.clear();
}

bool Command::remove_subcommand(const std::string& name) {
	std::lock_guard guard(mutex_);
	return !!subcommands_.erase(name);
}

void Command::clear_subcommands() {
	std::lock_guard guard(mutex_);
	subcommands_.clear();
}

} // commands, ember