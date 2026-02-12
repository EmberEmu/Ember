/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Command.h"
#include <algorithm>
#include <cassert>

namespace ember::commands {

Command::Command(std::string name) : name_(name) {}

auto Command::validate_arg_count(const std::size_t count) const -> Result {
	if(count < required_arg_count()) {
		return Result::missing_args;
	} else if(count > args_.size()) {
		return Result::too_many_args;
	}

	return Result::success;
}

ArgumentStore Command::build_argument_store(std::span<const std::string> tokens) const {
	ArgumentStore arg_store;

	assert(args_.size() >= tokens.size() && "bad token input");

	for(auto i = 0u; i < tokens.size(); ++i) {
		Argument arg(tokens[i], args_[i].required);
		arg_store.emplace(args_[i].value, std::move(arg));
	}

	return arg_store;
}

Result Command::execute(std::span<const std::string> arguments) {
	std::shared_ptr<CommandHandler> handler;
	ArgumentStore arg_store;

	{
		std::lock_guard guard(mutex_);

		if(auto validation = validate_arg_count(arguments.size()); validation != Result::success) {
			return validation;
		}

		arg_store = std::move(build_argument_store(arguments));

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

Command& Command::arg(std::string argument) {
	std::lock_guard guard(mutex_);

	if(optional_arg_count() > 0) {
		throw std::invalid_argument("Required arguments must be placed before optional arguments");
	}
		
	args_.emplace_back(std::move(argument), true);
	return *this;
}

Command& Command::optional_arg(std::string argument) {
	std::lock_guard guard(mutex_);

	args_.emplace_back(std::move(argument), false);
	return *this;
}

Command& Command::description(std::string description) {
	std::lock_guard guard(mutex_);

	description_ = std::move(description);
	return *this;
}

Command& Command::handler(CommandHandler handler) {
	std::lock_guard guard(mutex_);

	// shared_ptr to avoid having to copy a potentially heavyweight handler
	// during invocation for thread safety reasons - guaranteeing the lifetime
	// of the handler rather than copying is sufficient
	handler_ = std::make_shared<CommandHandler>(handler);
	return *this;
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

	for(const auto& arg : args_) {
		result += (arg.required ? " <" : " [") + arg.value + (arg.required ? ">" : "]");
	}

	return result;
}

std::shared_ptr<Command> Command::subcommand(std::string name) {
	std::lock_guard guard(mutex_);

	auto [entry, _] = subcommands_.insert_or_assign(
		name, std::make_shared<Command>(name)
	);

	return entry->second;
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