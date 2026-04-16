/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CommandHelpers.h"
#include <commands/Commands.h>
#include <logger/CommandSink.h>
#include <logger/Logger.h>
#include <boost/lexical_cast.hpp>
#include <chrono>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ember::utility {

void log_subcommands(log::Logger& logger, const std::string& path, const commands::CommandMap& subcommands) {
	LOG_CONSOLE(logger, R"(Available subcomands for "{}": )", path);

	for(auto& subcommand : subcommands | std::views::values) {
		LOG_CONSOLE(logger, "{} : {}", subcommand->name(), subcommand->description());
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

bool convert_bool(const std::string_view token) {
	if(token == "on" || token == "enable" || token == "1" || token == "true") {
		return true;
	} else if(token == "off" || token == "disable" || token == "0" || token == "false") {
		return false;
	}

	throw std::runtime_error("Unable to convert bool argument");
}

std::any convert_type(const std::type_info& info, std::string_view token) {
	if(info == typeid(std::string)) {
		return std::string(token);
	} else if(info == typeid(char)) {
		return boost::lexical_cast<char>(token);
	} else if(info == typeid(std::uint8_t)) {
		return boost::lexical_cast<std::uint8_t>(token);
	} else if(info == typeid(std::uint16_t)) {
		return boost::lexical_cast<std::uint16_t>(token);
	} else if(info == typeid(std::uint32_t)) {
		return boost::lexical_cast<std::uint32_t>(token);
	} else if(info == typeid(std::uint64_t)) {
		return boost::lexical_cast<std::uint64_t>(token);
	} else if(info == typeid(std::int8_t)) {
		return boost::lexical_cast<std::int8_t>(token);
	} else if(info == typeid(std::int16_t)) {
		return boost::lexical_cast<std::int16_t>(token);
	} else if(info == typeid(std::int32_t)) {
		return boost::lexical_cast<std::int32_t>(token);
	} else if(info == typeid(std::int64_t)) {
		return boost::lexical_cast<std::int64_t>(token);
	} else if(info == typeid(float)) {
		return boost::lexical_cast<float>(token);
	} else if(info == typeid(double)) {
		return boost::lexical_cast<double>(token);
	} else if(info == typeid(bool)) {
		return convert_bool(token);
	} else if(info == typeid(std::chrono::seconds)) {
		return std::chrono::seconds(boost::lexical_cast<int>(token));
	} else {
		throw std::runtime_error("Unhandled argument type");
	}
}

void execute_command(const std::string_view input, const commands::Command& root, log::Logger& logger) try {
	const auto tokens = commands::parse_input(input);
	const auto search = root.find(tokens);

	if(tokens.empty()) {
		LOG_CONERR(logger, "Command cannot be empty");
		return;
	}

	if(!search.command) {
		LOG_CONERR(logger, R"(Command "{}" not found)", tokens.front());
		return;
	}

	auto arguments = std::span(tokens).subspan(search.depth); // discard command name token

	/*
	 * The execution handler validates argument count but the way we iterate for type conversion
	 * means any unused tokens will be ignored, so the check for too many arguments will never
	 * trigger from here. This check is only for user feedback, it is not required for correct behaviour.
	 */
	if(arguments.size() > search.command->argument_count()) {
		LOG_CONERR(
			logger,
			R"(Too many arguments passed to "{}" (takes {}, got {}))",
			tokens.front(),
			search.command->argument_count(),
			arguments.size()
		);

		return;
	}

	// argument type conversion
	std::vector<std::any> arg_values;
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
	LOG_CONERR(logger, R"(Error parsing command arguments, "{}")", e.what());
} catch(const boost::bad_lexical_cast&) {
	LOG_CONERR(logger, R"(Unable to execute command, invalid argument types provided)");
} catch(const std::exception& e) {
	LOG_CONERR(logger, R"(Error during command execution, "{}")", e.what());
}

void handle_help_command(const commands::Arguments& arguments, const commands::Command& root, log::Logger& logger) {
	if(arguments.empty()) {
		LOG_CONSOLE(
			logger,
			R"(Type "help "<command>" (quoted) to display command usage or press tab for autocompletion)"
		);

		return;
	}

	const auto command = arguments["command"].as<std::string>();
	const auto tokens = commands::parse_input(command);
	const auto result = root.find(tokens);

	if(result.command) {
		LOG_CONSOLE(logger, "Usage: {} {}", command, result.command->usage_string());
		LOG_CONSOLE(logger, "Description: {}", result.command->description());
	} else {
		LOG_CONERR(logger, R"(Command "{}" not found)", command);
	}
}

std::string suggest_command(const commands::Command& root, const std::string_view cmd) {
	auto results = root.autocomplete(cmd);

	if(!results.substring.empty()) {
		return results.substring;
	} else {
		return {};
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

void register_common_commands(commands::Command& root, log::Logger& logger) {
	root.insert("help")
		->description("Display console command usage information")
		->argument<std::string>("command", commands::optional)
		->handler([&](const auto& arguments) {
			handle_help_command(arguments, root, logger);
		});

#ifdef _WIN32
	root.insert("cls")
		->description("Clears the console")
		->handler([&](const auto&) {
			handle_cls_command(logger);
		});
#endif
}

void register_command_handlers(commands::Command& root, log::Logger& logger, const bool allow_suggest) {
#ifdef _WIN32
	auto sinks = logger.fetch_sink(log::CommandSink::sink_name);

	if(sinks.empty()) {
		SLOG_INFO(logger, "Console commands disabled, no suitable logging sink found");
		return;
	}

	auto sink = static_cast<log::CommandSink*>(sinks.front().get());

	sink->register_autocomplete([&](auto cmd) {
		return root.autocomplete(cmd);
	});

	sink->register_handler([&](auto input) {
		execute_command(input, root, logger);
	});

	if(allow_suggest) {
		sink->register_suggestion([&](auto cmd) {
			return suggest_command(root, cmd);
		});
	}
#endif
}

} // utility, ember