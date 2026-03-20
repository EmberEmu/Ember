/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CommandHelpers.h"
#include <logger/CommandSink.h>
#include <logger/Logger.h>
#include <commands/Registry.h>
#include <commands/Utility.h>
#include <shared/utility/CommandHelpers.h>
#include <boost/lexical_cast/bad_lexical_cast.hpp>
#include <format>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ember::fusion {

void log_subcommands(log::Logger& logger, const std::string& path, const commands::CommandMap& subcommands) {
	LOG_CONSOLE_ASYNC(logger, R"(Available subcomands for "{}": )", path);

	for(auto& subcommand : subcommands | std::views::values) {
		LOG_CONSOLE_ASYNC(logger, "{} : {}", subcommand->name(), subcommand->description());
	}
}

void handle_command_result(commands::Result result,
                           const std::string& path,
                           const commands::Command& command,
                           log::Logger& logger) {
    switch(result) {
        case commands::Result::missing_args:
			[[fallthrough]];
        case commands::Result::too_many_args:
			LOG_CONSOLE_ERROR_ASYNC(logger, "Usage: {} {}", path, command.usage_string());
            break;
        case commands::Result::subcommands:
			log_subcommands(logger, path, command.commands());
            break;
        case commands::Result::unavailable:
			LOG_CONSOLE_ERROR_ASYNC(logger, R"(Command "{}" is currently unavailable.)", path);
            break;
		case commands::Result::invalid_types:
			LOG_CONSOLE_ERROR_ASYNC(logger, R"(Bad argument types when attempting to execute "{}")", path);
			break;
        default:
			LOG_CONSOLE_ERROR_ASYNC(logger, R"(An unhandled error occurred while executing "{}")", path);
            break;
    }
}

void execute_command(const std::string_view input, const Registries& registries, log::Logger& logger) try {
	const auto tokens = commands::Registry::parse_input(input);

	// figure out which registry to use
	auto prefix = tokens.front();
	const auto it = registries.find(prefix);

	std::span token_view = tokens;

	if(token_view.size() == 1 || !registries.contains(prefix)) {
		prefix = "root"; // use the root registry
	} else {
		token_view = token_view.subspan<1>();
	}

	const auto& registry = registries.at(prefix);
	const auto search = registry.find(token_view);

	if(!search.command) {
		LOG_CONSOLE_ERROR_ASYNC(
			logger,
			R"(Command "{}" not found)",
			token_view.front()
		);

		return;
	}

	auto arguments = token_view.subspan(search.depth); // discard command name token

	/*
	 * The execution handler validates argument count but the way we iterate for type conversion
	 * means any unused tokens will be ignored, so the check for too many arguments will never
	 * trigger from here. This check is only for user feedback, it is not required for correct behaviour.
	 */
	if(arguments.size() > search.command->argument_count()) {
		LOG_CONSOLE_ERROR_ASYNC(
			logger,
			R"(Too many arguments passed to "{}" (takes {}, got {}))",
			token_view.front(),
			search.command->argument_count(),
			arguments.size()
		);

		return;
	}

	// argument type conversion
	std::vector<std::any> arg_values;
	auto command_args = search.command->arguments();

	for(auto [expected, argument] : std::views::zip(command_args, arguments)) {
		arg_values.emplace_back(utility::convert_type(*expected.type, argument));
	}

	// execute the command handler
	if(auto result = search.command->execute(arg_values); result != commands::Result::success) {
		const auto& path = commands::path_fragment(token_view, search.depth);
		handle_command_result(result, path, *search.command, logger);
	}
} catch(const commands::parse_error& e) {
	LOG_CONSOLE_ERROR_ASYNC(logger, R"(Error parsing command arguments, "{}")", e.what());
} catch(const boost::bad_lexical_cast&) {
	LOG_CONSOLE_ERROR_ASYNC(logger, R"(Unable to execute command, invalid argument types provided)");
} catch(const std::exception& e) {
	LOG_CONSOLE_ERROR_ASYNC(logger, R"(Error during command execution, "{}")", e.what());
}

void handle_help_command(const commands::Arguments& arguments,
                         const Registries& registries,
                         log::Logger& logger) {
	if(arguments.empty()) {
		LOG_CONSOLE_ASYNC(
			logger,
			R"(Type "help "<command>" (quoted) to display command usage or press tab for autocompletion)"
		);

		return;
	}

	const auto& command = arguments["command"].as<std::string>();
	const auto tokens = commands::Registry::parse_input(command);

	// figure out which registry to use
	auto prefix = tokens.front();
	const auto it = registries.find(prefix);

	std::span token_view = tokens;

	if(token_view.size() == 1 || !registries.contains(prefix)) {
		prefix = "root"; // use the root registry
	} else {
		token_view = token_view.subspan<1>();
	}

	const auto& registry = registries.at(prefix);
	const auto result = registry.find(token_view);

	if(result.command) {
		LOG_CONSOLE_ASYNC(logger, "Usage: {} {}", command, result.command->usage_string());
		LOG_CONSOLE_ASYNC(logger, "Description: {}", result.command->description());
	} else {
		LOG_CONSOLE_ERROR_ASYNC(logger, R"(Command "{}" not found)", command);
	}
}

#ifdef _WIN32
void handle_cls_command(log::Logger& logger) {
	auto sinks = logger.fetch_sink(log::CommandSink::sink_name);

	if(sinks.empty()) {
		LOG_ERROR_SYNC(logger, "Could not locate a command sink, cannot execute command");
	}

	assert(sinks.size() == 1 && "multiple command sinks?");

	auto command_sink = static_cast<log::CommandSink*>(sinks.front().get());
	assert(command_sink->name() == log::CommandSink::sink_name && "unexpected sink name");
	command_sink->clear_console();
}
#endif

void register_shared_commands(Registries& registries, log::Logger& logger) {
	auto& root = registries["root"];

	root.insert("help")
		->description("Display console command usage information")
		->optional_argument<std::string>("command")
		->handler([&](const commands::Arguments& arguments) {
			handle_help_command(arguments, registries, logger);
		});

#ifdef _WIN32
	root.insert("cls")
		->description("Clears the console")
		->handler([&](const auto&) {
			handle_cls_command(logger);
		});
#endif
}

std::string shortest_common_substring(std::span<const std::string> candidates) {
	std::string match;

	if(candidates.empty()) {
		return {};
	} else {
		match = candidates.front();
	}

	for(const auto& str : candidates) {
		std::size_t i = 0;

		while(i < match.size() && i < str.size() && match[i] == str[i]) {
			++i;
		}

		match.resize(i);

		if(match.empty()) {
			break;
		}
	}

	return match;
}

commands::Suggestions handle_autocomplete(Registries& registries, const std::string_view command) {
	auto tokens = commands::Registry::parse_input(command);

	// figure out which registry this command could belong to
	std::vector<std::pair<std::string, const commands::Registry*>> candidates;

	for(const auto& it : registries) {
		const auto& [key, value] = it;

		if(tokens.empty()) {
			candidates.emplace_back(key, &value);
		} else if(key == "root" || key.starts_with(tokens.front())) {
			candidates.emplace_back(key, &value);
		}
	}

	std::vector<std::string> completions;
	std::vector<commands::Suggestions::Record> records;
	std::string_view split_command;
	std::string_view registry_name;

	auto pos = command.find_first_of(' ');

	if(pos == command.npos || pos == command.size() - 1) {
		split_command = "";
	} else {
		registry_name = registry_name.substr(0, pos);
		split_command = command.substr(pos + 1, command.size() - pos);
	}

	for(auto& [name, registry]  : candidates) {
		std::string_view command_view = command;

		if(name != "root") {
			command_view = split_command;
		}

		// exclude current registry from the search if the prefix doesn't belong to it
		if(!registry_name.empty() && registry_name != name) {
			continue;
		}

		auto suggestions = registry->autocomplete(command_view);
	
		// populate auto-completion candidate names for these matches
		for(const auto& record : suggestions.records) {
			if(name != "root") {
				completions.emplace_back(std::format("{} {}", name, record.name));
			} else {
				completions.emplace_back(record.name);
			}
		}

		// populate command records, prefixing the current registry name for the table output
		for(auto& suggestion : suggestions.records) {
			if(name != "root") {
				suggestion.name = std::format("{} {}", name, suggestion.name);
			}
		}

		records.insert_range(records.begin(), suggestions.records);
	}

	auto substring = shortest_common_substring(completions);

	return commands::Suggestions {
		.substring = std::move(substring),
		.records = std::move(records),
	};
}

void register_command_handlers(Registries& registries, log::Logger& logger) {
#ifdef _WIN32
	auto sinks = logger.fetch_sink(log::CommandSink::sink_name);

	if(sinks.empty()) {
		LOG_INFO_SYNC(logger, "Console commands disabled, no suitable logging sink found");
		return;
	} else if(sinks.size() > 1) {
		LOG_ERROR_SYNC(logger, "Console commands disabled, multiple command logging sinks found");
		return;
	}

	auto sink = static_cast<log::CommandSink*>(sinks.front().get());

	sink->register_autocomplete([&](auto cmd) {
		return handle_autocomplete(registries, cmd);
	});

	sink->register_handler([&](auto input) {
		execute_command(input, registries, logger);
	});
#endif
}

} // fusion, ember