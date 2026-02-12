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

namespace ember::commands {

Registry::Registry()
	: root_("") {}

std::shared_ptr<Command> Registry::register_command(std::string name) {
	std::lock_guard guard(lock_);

	auto command = std::make_shared<Command>(name);
	return root_.subcommand(name);
}

std::vector<std::string> Registry::parse_input(std::string_view input) const {
	std::vector<std::string> arg_values;
	std::string input_str(input);
	std::istringstream stream;
	stream.str(std::move(input_str));
	
	std::string token;

	while(stream >> token) {
		arg_values.emplace_back(std::move(token));
		token.clear(); // reset state
	}

	return arg_values;
}

// this is really inefficient due to the registry copies (must be copied from subcommands for safety)
// but it's not even remotely performance sensitive, so it'll do
std::shared_ptr<Command> Registry::find_command(std::span<const std::string> tokens, std::size_t& depth) {
	// we need go as deep as possible into the subcommand chain
	std::shared_ptr<Command> command;
	auto registry = root_.subcommands();

	for(auto& token : tokens) {
		const auto& command_name = token;
		const auto result = registry.find(command_name);

		if(result != registry.end()) {
			++depth;
			command = result->second;
			registry = command->subcommands();
		} else {
			break;
		}
	}

	return command;
}

auto Registry::get(std::span<const std::string> tokens) -> CommandGet {
	std::lock_guard guard(lock_);
	std::size_t depth = 0;
	return { find_command(tokens, depth), depth };
}

std::string Registry::longest_prefix(std::span<const Suggestions::Record> matches) const {
	if(matches.empty()) {
		return {};
	}

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

Suggestions Registry::autocomplete_recurse(const CommandMap& commands, std::span<const std::string> tokens) const {
	Suggestions result;

	if(tokens.empty()) {
		for(const auto& cmd : commands | std::views::values) {
			result.records.emplace_back(cmd->name(), cmd->description());
		}

		return result;
	}

	for(const auto& cmd : commands | std::views::values) {
		if(cmd->name().starts_with(tokens.front())) {
			result.records.emplace_back(cmd->name(), cmd->description());
		}
	}

	// hack to find the longest common substring without
	// bothering to write an entire trie (this is not perf. sensitive)
	if(result.records.empty()) {
		return result;
	}

	result.substring = longest_prefix(result.records);

	if(auto it = commands.find(result.substring); it != commands.end()) {
		const auto& [_, command] = *it;

		if(!command->subcommands().empty()) {
			// We'll only update the suggestions if we already had an exact match in the query.
			// This means that this autocomplete will display info on the current command,
			// not subcommands that match the new query string that we're about to return.
			const bool exact_match = commands.contains(tokens.front());

			if(exact_match) {
				auto recurse_res = autocomplete_recurse(command->subcommands(), tokens.subspan<1>());
				result.substring.push_back(' ');
				result.substring += recurse_res.substring;
				result.records = std::move(recurse_res.records);
			} else {
				result.substring.push_back(' ');
			}
		}
	}

	return result;
}

Suggestions Registry::autocomplete(std::string_view query) const {
	std::lock_guard guard(lock_);
	
	auto tokens = parse_input(query);
	auto results = autocomplete_recurse(root_.subcommands(), tokens);

	std::ranges::sort(results.records, [&](const auto& a, const auto& b) {
		return a.name < b.name;
	});
	
	return results;
}

bool Registry::unregister(const std::string& name) {
	std::lock_guard guard(lock_);
	return !!root_.remove_subcommand(name);
}

} // commands, ember