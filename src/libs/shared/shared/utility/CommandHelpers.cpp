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
#include <boost/lexical_cast.hpp>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ember::utility {

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
			LOG_CONSOLE_ERROR_ASYNC(logger, "Usage: {}{}", path, command.usage_string());
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

commands::args::Value convert_type(commands::args::Type type, std::string_view token) {
	switch(type) {
		case commands::args::Type::at_char:
			return boost::lexical_cast<char>(token);
			break;
		case commands::args::Type::at_string:
			return boost::lexical_cast<std::string>(token);
			break;
		case commands::args::Type::at_uint8:
			return boost::lexical_cast<std::uint8_t>(token);
			break;
		case commands::args::Type::at_uint16:
			return boost::lexical_cast<std::uint16_t>(token);
			break;
		case commands::args::Type::at_uint32:
			return boost::lexical_cast<std::uint32_t>(token);
			break;
		case commands::args::Type::at_uint64:
			return boost::lexical_cast<std::uint64_t>(token);
			break;
		case commands::args::Type::at_int8:
			return boost::lexical_cast<std::int8_t>(token);
			break;
		case commands::args::Type::at_int16:
			return boost::lexical_cast<std::int16_t>(token);
			break;
		case commands::args::Type::at_int32:
			return boost::lexical_cast<std::int32_t>(token);
			break;
		case commands::args::Type::at_int64:
			return boost::lexical_cast<std::int64_t>(token);
			break;
		case commands::args::Type::at_float:
			return boost::lexical_cast<float>(token);
			break;
		case commands::args::Type::at_double:
			return boost::lexical_cast<double>(token);
			break;
		default:
			throw exception("Unhandled argument type");
	}
}

void execute_command(const std::string_view input, const commands::Registry& registry, log::Logger& logger) try {
	const auto tokens = registry.parse_input(input);
	const auto search = registry.find(tokens);

	if(!search.command) {
		LOG_CONSOLE_ERROR_ASYNC(
			logger,
			R"(Command "{}" not found)",
			tokens.front()
		);

		return;
	}

	auto arguments = std::span(tokens).subspan(search.depth); // discard command name token

	/*
	 * The execution handler validates argument count but the way we iterate for type conversion
	 * means any unused tokens will be ignored, so the check for too many arguments will never
	 * trigger from here. This check is only for user feedback, it is not required for correct behaviour.
	 */
	if(arguments.size() > search.command->argument_count()) {
		LOG_CONSOLE_ERROR_ASYNC(
			logger,
			R"(Too many arguments passed to "{}" (takes {}, got {}))",
			tokens.front(),
			search.command->argument_count(),
			arguments.size()
		);

		return;
	}

	// argument type conversion
	std::vector<commands::args::Value> arg_values;
	auto command_args = search.command->arguments();

	for(auto [expected, argument] : std::views::zip(command_args, arguments)) {
		arg_values.emplace_back(convert_type(expected.type, argument));
	}

	// execute the command handler
	if(auto result = search.command->execute(arg_values); result != commands::Result::success) {
		const auto& path = commands::path_fragment(tokens, search.depth);
		handle_command_result(result, path, *search.command, logger);
	}
} catch(const commands::parse_error& e) {
	LOG_CONSOLE_ERROR_ASYNC(logger, R"(Error parsing command arguments, "{}")", e.what());
} catch(const boost::bad_lexical_cast&) {
	LOG_CONSOLE_ERROR_ASYNC(logger, R"(Unable to execute command, invalid argument types provided)");
} catch(const std::exception& e) {
	LOG_CONSOLE_ERROR_ASYNC(logger, R"(Error during command execution, "{}")", e.what());
}

void register_shared_commands(commands::Registry& registry, log::Logger& logger) {
	registry.insert("help")
		->description("Display console command usage information")
		->optional_argument("command", commands::args::Type::at_string)
		->handler([&](const auto& arguments) {
			if(arguments.empty()) {
				LOG_CONSOLE_ASYNC(
					logger,
					R"(Type "help "<command>" (quoted) to display command usage or press tab for autocompletion)"
				);

				return;
			}

			const auto command = std::get<std::string>(arguments.at("command"));
			const auto tokens = commands::Registry::parse_input(command);
			const auto result = registry.find(tokens);

			if(result.command) {
				LOG_CONSOLE_ASYNC(logger, "Usage: {} {}", command, result.command->usage_string());
				LOG_CONSOLE_ASYNC(logger, "Description: {}", result.command->description());
			} else {
				LOG_CONSOLE_ERROR_ASYNC(logger, R"(Command "{}" not found)", command);
			}
		});

#ifdef _WIN32
	registry.insert("cls")
		->description("Clears the console")
		->handler([&](const auto& arguments) {
			auto sinks = logger.fetch_sink(log::CommandSink::sink_name);

			if(sinks.empty()) {
				LOG_ERROR_SYNC(logger, "Could not locate a command sink, cannot execute command");
			}

			assert(sinks.size() == 1 && "multiple command sinks?");

			auto command_sink = static_cast<log::CommandSink*>(sinks.front().get());
			assert(command_sink->name() == log::CommandSink::sink_name && "unexpected sink name");
			command_sink->clear_console();
	});
#endif
}

void register_command_handlers(commands::Registry& registry, log::Logger& logger) {
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
		return registry.autocomplete(cmd);
	});

	sink->register_handler([&](auto input) {
		execute_command(input, registry, logger);
	});
#endif
}

} // utility, ember