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
	LOG_CONSOLE(logger, R"(Available subcomands for "{}": )", path);

	for(auto& subcommand : subcommands | std::views::values) {
		LOG_CONSOLE(logger, "{} : {}", subcommand->name(), subcommand->description());
	}
}

void handle_command_result(commands::Result result, const std::string& path,
                           const commands::Command& command, log::Logger& logger) {
    switch(result) {
        case commands::Result::missing_args:
			[[fallthrough]];
        case commands::Result::too_many_args:
			LOG_CONERR(logger, "Usage: {} {}", path, command.usage_string());
            break;
        case commands::Result::subcommands:
			log_subcommands(logger, path, command.commands());
            break;
        case commands::Result::unavailable:
			LOG_CONERR(logger, R"(Command "{}" is currently unavailable.)", path);
            break;
		case commands::Result::invalid_types:
			LOG_CONERR(logger, R"(Bad argument types when attempting to execute "{}")", path);
			break;
        default:
			LOG_CONERR(logger, R"(An unhandled error occurred while executing "{}")", path);
            break;
    }
}

void execute_command(const std::string_view input, const Registries& registries, log::Logger& logger) try {
	const auto tokens = commands::Registry::parse_input(input);

	if(tokens.empty()) {
		return;
	}

	// figure out which registry to use
	auto prefix = tokens.front();
	std::span token_view = tokens;
	token_view = token_view.subspan<1>();

	const auto it = registries.find(prefix);
	
	if(it == registries.end()) {
		LOG_CONERR(logger, R"(Command "{}" not found)", input);
		return;
	}

	const auto& registry = it->second;
	const auto search = registry.find(token_view);

	if(!search.command) {
		LOG_CONERR(logger, R"(Command "{}" not found)", input);
		return;
	}

	auto arguments = token_view.subspan(search.depth); // discard command name token

	/*
	 * The execution handler validates argument count but the way we iterate for type conversion
	 * means any unused tokens will be ignored, so the check for too many arguments will never
	 * trigger from here. This check is only for user feedback, it is not required for correct behaviour.
	 */
	if(arguments.size() > search.command->argument_count()) {
		LOG_CONERR(
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
		arg_values.emplace_back(utility::convert_type(expected.type, argument));
	}

	// execute the command handler
	if(auto result = search.command->execute(arg_values); result != commands::Result::success) {
		const auto& path = commands::path_fragment(token_view, search.depth);
		handle_command_result(result, path, *search.command, logger);
	}
} catch(const commands::parse_error& e) {
	LOG_CONERR(logger, R"(Error parsing command arguments, "{}")", e.what());
} catch(const boost::bad_lexical_cast&) {
	LOG_CONERR(logger, R"(Unable to execute command, invalid argument types provided)");
} catch(const std::exception& e) {
	LOG_CONERR(logger, R"(Error during command execution, "{}")", e.what());
}

void handle_help_command(const commands::Arguments& arguments,
                         const Registries& registries,
                         log::Logger& logger) {
	if(arguments.empty()) {
		LOG_CONSOLE(
			logger,
			R"(Type "help "<command>" (quoted) to display command usage or press tab for autocompletion)"
		);

		return;
	}

	const auto& command = arguments["command"].as<std::string>();
	const auto tokens = commands::Registry::parse_input(command);

	// figure out which registry to use
	auto prefix = tokens.front();
	std::span token_view = tokens;
	token_view = token_view.subspan<1>();

	if(auto it = registries.find(prefix); it != registries.end()) {
		const auto result = it->second.find(token_view);

		if(result.command) {
			LOG_CONSOLE(logger, "Usage: {} {}", command, result.command->usage_string());
			LOG_CONSOLE(logger, "Description: {}", result.command->description());
		} else {
			LOG_CONERR(logger, R"(Command "{}" not found)", command);
		}
	} else {
		LOG_CONERR(logger, R"(Command "{} {}" not found)", prefix, command);
	}
}

#ifdef _WIN32
void handle_cls_command(log::Logger& logger) {
	auto sinks = logger.fetch_sink(log::CommandSink::sink_name);

	if(sinks.empty()) {
		SLOG_ERROR(logger, "Could not locate a command sink, cannot execute command");
	}

	assert(sinks.size() == 1 && "multiple command sinks?");

	auto command_sink = static_cast<log::CommandSink*>(sinks.front().get());
	assert(command_sink->name() == log::CommandSink::sink_name && "unexpected sink name");
	command_sink->clear_console();
}
#endif

void register_shared_commands(Registries& registries, log::Logger& logger) {
	auto& root = registries["fusion"];

	root.insert("help")
		->description("Display console command usage information")
		->argument<std::string>("command", commands::optional)
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

std::string shortest_common_substring(std::span<const std::string_view> candidates) {
	std::string match;

	if(candidates.empty()) {
		return {};
	} else {
		match = candidates.front();
	}

	for(auto& str : candidates) {
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

commands::Suggestions full_command_table(const Registries& registries) {
	commands::Suggestions result;

	for(const auto& registry : registries) {
		const auto& [k, v] = registry;
		auto suggestions = v.autocomplete("");
		
	
		for(auto& suggestion : suggestions.records) {
			suggestion.name = std::format("{} {}", k, suggestion.name);
		}

		result.records.insert_range(result.records.end(), std::move(suggestions.records));
	}

	return result;
}

commands::Suggestions handle_autocomplete(const Registries& registries, const std::string_view command) {
	auto tokens = commands::Registry::parse_input(command);

	// delegate to a specific service registry if we can
	if(!tokens.empty()) {
		for(auto& entry : registries) {
			const auto& [name, registry] = entry;

			if(tokens.front() == name) {
				auto subcommand = command.substr(command.find_first_of(name) + name.size(), -1);

				if(subcommand.starts_with(' ')) {
					subcommand = subcommand.substr(1, -1);
				}

				auto result = registry.autocomplete(subcommand);
				result.substring = std::format("{} {}", name, result.substring);
				return result;
			}
		}
	}

	// nope, we only had a partial match, build the table ourselves
	auto full_table = full_command_table(registries);
	std::vector<std::string_view> completions;

	for(auto& record : full_table.records) {
		if(record.name.starts_with(command)) {
			completions.emplace_back(record.name);
		}
	}

	full_table.substring = shortest_common_substring(completions);

	std::erase_if(full_table.records, [&](const auto& record) {
		return !record.name.starts_with(full_table.substring);
	});

	return full_table;
}

std::string suggest_command(const Registries& registries, const std::string_view cmd) {
	auto results = handle_autocomplete(registries, cmd);

	if(!results.substring.empty()) {
		return results.substring;
	} else {
		return {};
	}
}

void register_command_handlers(Registries& registries, log::Logger& logger) {
#ifdef _WIN32
	auto sinks = logger.fetch_sink(log::CommandSink::sink_name);

	if(sinks.empty()) {
		SLOG_INFO(logger, "Console commands disabled, no suitable logging sink found");
		return;
	} else if(sinks.size() > 1) {
		SLOG_ERROR(logger, "Console commands disabled, multiple command logging sinks found");
		return;
	}

	auto sink = static_cast<log::CommandSink*>(sinks.front().get());

	sink->register_autocomplete([&](auto cmd) {
		return handle_autocomplete(registries, cmd);
	});

	sink->register_handler([&](auto input) {
		execute_command(input, registries, logger);
	});

	sink->register_suggestion([&](auto cmd) {
		return suggest_command(registries, cmd);
	});
#endif
}

} // fusion, ember