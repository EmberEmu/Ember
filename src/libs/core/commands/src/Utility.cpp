/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <commands/Utility.h>
#include <commands/Command.h>
#include <boost/tokenizer.hpp>
#include <algorithm>
#include <ranges>

namespace ember::commands {

std::shared_ptr<Command> create(std::string name) {
	return Command::create(name);
}

std::string path_fragment(std::span<const std::string> tokens, std::size_t depth) {
	constexpr auto approx_cmd_len = 10u;

	if(tokens.size() < depth) {
		return {};
	}

	std::string fragment;
	fragment.reserve(depth * approx_cmd_len);

	for(auto i = 0; i < depth; ++i) {
		fragment += tokens[i];

		if(i != depth - 1) {
			fragment += " ";
		}
	}

	return fragment;
}

std::vector<std::string> parse_input(const std::string_view input, bool escape) {
	std::vector<std::string> tokens;
	std::string str(input);

	try {
		boost::escaped_list_separator<char> sep;

		if(escape) {
			sep = boost::escaped_list_separator<char>('\\', ' ', '"');
		} else {
			sep = boost::escaped_list_separator<char>(' ');
		}

		boost::tokenizer<boost::escaped_list_separator<char>> tok(str, sep);
		tokens.assign_range(tok);
	} catch(boost::escaped_list_error& e) {
		throw parse_error(e.what());
	}

	return tokens;
}

namespace impl {

/*
 * 'hack' to find the longest common substring without bothering to write an entire trie (this is not perf. sensitive)
 */
std::string longest_prefix(std::span<const Suggestions::Record> matches) {
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

Suggestions autocomplete_recurse(const CommandMap& commands, std::span<const std::string> tokens) {
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

	if(result.records.empty()) {
		return result;
	}

	result.substring = longest_prefix(result.records);

	if(auto it = commands.find(result.substring); it != commands.end()) {
		const auto& [_, command] = *it;

		if(!command->commands().empty()) {
			// We'll only update the suggestions if we already had an exact match in the query.
			// This means that this autocomplete will display info on the current command,
			// not subcommands that match the new query string that we're about to return.
			const bool exact_match = commands.contains(tokens.front());

			if(exact_match) {
				auto recurse_res = autocomplete_recurse(command->commands(), tokens.subspan<1>());
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

} // impl

} // commands, ember