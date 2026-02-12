/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Registry.h"
#include <algorithm>
#include <ranges>
#include <cassert>

#include <iostream>

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
	std::vector<std::string> arg_values;
	std::istringstream stream(input);
	std::string token;

	while(stream >> token) {
		arg_values.emplace_back(std::move(token));
		token.clear(); // reset state
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

	if(auto validation = validate_arg_count(tokens.size(), command); validation != Result::Success) {
		return validation;
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

std::string Registry::longest_prefix(std::span<const PartialMatches::Record> matches) const {
	std::string prefix = matches.front().name;

	for(const auto& match : matches) {
		std::size_t i = 0;

		while(i < prefix.size() && i < match.name.size() && prefix[i] == match.name[i]) {
			++i;
		}

		prefix.resize(i);

		if(prefix.empty()) {
			break;
		}
	}

	return prefix;
}

PartialMatches Registry::autocomplete_recurse(const std::unordered_map<std::string, Command>& commands, std::span<const std::string> tokens) const {
	PartialMatches result;

	if(tokens.empty()) {
		for(const auto& cmd : commands | std::views::values) {
			result.records.emplace_back(cmd.name_, cmd.description_);
		}

		return result;
	}

	for(const auto& cmd : commands | std::views::values) {
		if(cmd.name_.starts_with(tokens.front())) {
			result.records.emplace_back(cmd.name_, cmd.description_);
		}
	}

	// hack to find the longest common substring without
	// bothering to write an entire trie (this is not perf. sensitive)
	if(result.records.empty()) {
		return result;
	}

	result.partial_match = longest_prefix(result.records);

	if(auto it = commands.find(result.partial_match); it != commands.end()) {
		const auto& [_, command] = *it;

		if(!command.subcommands_.empty()) {
			// We'll only update the suggestions if we already had an exact match in the query.
			// This means that this autocomplete will display info on the current command,
			// not subcommands that match the new query string that we're about to return.
			const bool exact_match = commands.contains(tokens.front());

			if(exact_match) {
				auto recurse_res = autocomplete_recurse(command.subcommands_, tokens.subspan<1>());
				result.partial_match.push_back(' ');
				result.partial_match += recurse_res.partial_match;
				result.records = recurse_res.records;
			} else {
				result.partial_match.push_back(' ');
			}
		}
	}

	std::ranges::sort(result.records, [&](const auto& a, const auto& b) {
		return a.name < b.name;
	});

	return result;
}

PartialMatches Registry::autocomplete(std::string_view query) const {
	std::lock_guard guard(lock_);
	
	auto tokens = parse_input(query);
	return autocomplete_recurse(registry_, tokens);
}

bool Registry::unregister(const std::string& name) {
	std::lock_guard guard(lock_);
	return !!registry_.erase(name);
}

} // commands, ember