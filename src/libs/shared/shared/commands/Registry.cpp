/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Registry.h"
#include <ranges>
#include <boost/tokenizer.hpp>
#include <cassert>

namespace ember::commands {

Command& Registry::register_command(std::string name) {
	std::lock_guard guard(lock_);

	auto [pair, _] = registry_.try_emplace(name, name);
	return pair->second;
}

std::vector<std::string> Registry::parse_input(std::string_view input) const {
	return parse_input(std::string(input));
}

std::vector<std::string> Registry::parse_input(const std::string& input) const {
	boost::char_separator<char> separator(" ");
	boost::tokenizer<boost::char_separator<char>> tokens(input, separator);
	std::vector<std::string> arg_values;

	for(const auto& token : tokens) {
		arg_values.emplace_back(token);
	}

	return arg_values;
}

auto Registry::execute(const std::string& input) -> Result {
	auto tokens = parse_input(input);
	return execute(tokens);
}

auto Registry::validate_arg_count(const std::size_t count, const Command& cmd) const -> Result {
	if(count < cmd.req_args) {
		return Result::MissingArgs;
	} else if(count > cmd.total_args) {
		return Result::TooManyArgs;
	}

	return Result::Success;
}

ArgumentStore Registry::build_argument_store(std::span<const std::string> tokens, const Command& command) {
	ArgumentStore arg_store;

	assert(command.args_.size() >= tokens.size());

	for(auto i = 0u; i < tokens.size(); ++i) {
		Argument arg(tokens[i], command.args_[i].required);
		arg_store.emplace(command.args_[i].value, std::move(arg));
	}
	
	return arg_store;
}

auto Registry::execute(std::span<const std::string> tokens) -> Result {
	if(tokens.empty()) {
		return Result::BadInput;
	}

	std::lock_guard guard(lock_);

	const auto& command_name = tokens.front();
	auto result = registry_.find(command_name);

	if(result == registry_.end()) {
		return Result::NotFound;
	}

	auto& [_, command] = *result;

	if(auto result = validate_arg_count(tokens.size(), command); result != Result::Success) {
		return result;
	}

	auto arg_store = build_argument_store(tokens, command);

	if(command.handler_) {
		command.handler_(std::move(arg_store));
		return Result::Success;
	} else {
		return Result::NoHandler;
	}
}

std::optional<Command> Registry::get(std::string name) {
	std::lock_guard guard(lock_);

	if(auto cmd = registry_.find(name); cmd != registry_.end()) {
		return cmd->second;
	} else {
		return std::nullopt;
	}
}

PartialMatches Registry::autocomplete(std::string_view query) const {
	std::lock_guard guard(lock_);

	PartialMatches result;
	result.partial_match = query;

	// find potential command matches
	for(const auto& cmd : registry_ | std::views::values) {
		if(cmd.name_.starts_with(query)) {
			result.records.emplace_back(cmd.name_, cmd.description_);
		}
	}

	// hack to find the longest common substring without
	// bothering to write an entire trie (this is not perf. sensitive)
	if(!result.records.empty()) {
		result.partial_match = result.records.front().name;
	}
	
	for(auto it = result.records.begin(); it != result.records.end();) {
		if(!it->name.starts_with(result.partial_match)) {
			result.partial_match.pop_back();
		} else {
			++it;
		}
	}

	std::ranges::sort(result.records, [&](const auto& a, const auto& b) {
		return a.name < b.name;
	});

	return result;
}

bool Registry::unregister(const std::string& name) {
	std::lock_guard guard(lock_);
	return !!registry_.erase(name);
}

} // commands, ember