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

namespace ember::commands {

Command& Registry::register_command(std::string name) {
	std::lock_guard guard(lock_);

	Command cmd(name);
	auto [f, s] = registry_.try_emplace(name, cmd);
	return f->second;
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

auto Registry::execute_command(const std::string& input) -> Result {
	auto tokens = parse_input(input);
	return execute_command(tokens);
}

auto Registry::validate_arg_count(const std::size_t count, const Command& cmd) const -> Result {
	if(count < cmd.req_args) {
		return Result::MissingArgs;
	}

	if(count > cmd.total_args) {
		return Result::TooManyArgs;
	}

	return Result::Success;
}

auto Registry::execute_command(const std::vector<std::string>& tokens) -> Result {
	if(tokens.empty()) {
		return Result::BadInput;
	}
	std::lock_guard guard(lock_);

	const auto& cmd_name = tokens.front();
	auto res = registry_.find(cmd_name);

	if(res == registry_.end()) {
		return Result::NotFound;
	}

	auto& [key, value] = *res;

	if(auto res = validate_arg_count(tokens.size(), value); res != Result::Success) {
		return res;
	}

	ArgumentStore arg_store;
	
	for(auto i = 0u; i < tokens.size(); ++i) {
		Argument arg(tokens[i], value.args_[i].required);
		arg_store.emplace(value.args_[i].value, std::move(arg));
	}

	if(value.handler_) {
		value.handler_(std::move(arg_store));
	} else {
		return Result::NoHandler;
	}

	return Result::Success;
}

std::optional<Command> Registry::get_command(std::string name) {
	std::lock_guard guard(lock_);

	auto cmd = registry_.find(name);

	if(cmd == registry_.end()) {
		return std::nullopt;
	}

	return cmd->second;
}

// todo, needs a lock!
std::vector<std::string> Registry::autocomplete(std::string& cmd_buffer) const {
	std::lock_guard guard(lock_);

	std::vector<std::string> matches;

	// find potential command matches
	for(const auto& cmd : registry_ | std::views::values) {
		const auto& name = cmd.args().begin()->value;

		if(name.starts_with(cmd_buffer)) {
			matches.emplace_back(name);
		}
	}

	// hack to find the shortest common substring without
	// bothering to write an entire trie (this is not perf. sensitive)
	if(!matches.empty()) {
		cmd_buffer = matches.front();
	}
	
	for(auto it = matches.begin(); it != matches.end();) {
		if(!it->starts_with(cmd_buffer)) {
			cmd_buffer.pop_back();
		} else {
			++it;
		}
	}

	std::ranges::sort(matches);
	return matches;
}

} // commands, ember