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
#include <shared/commands/Registry.h>
#include <shared/commands/Utility.h>
#include <boost/lexical_cast.hpp>
#include <ranges>
#include <span>
#include <string>
#include <vector>

namespace ember::utility {

void log_subcommands(log::Logger& logger, const std::string& path, const commands::CommandMap& subcommands) {
	LOG_CONSOLE_ASYNC(logger, R"(Available subcomands for "{}": )", path);

	for(auto& subcommand : subcommands | std::views::values) {
		LOG_CONSOLE_ASYNC(logger, "{} : {}", subcommand->name(), subcommand->description());
	}
}

void handle_command_result(commands::Result result, const std::string& path, const commands::Command& command, log::Logger& logger) {
    switch(result) {
        case commands::Result::missing_args:
			[[fallthrough]];
        case commands::Result::too_many_args:
			LOG_CONSOLE_ERROR_ASYNC(logger, "Usage: {}{}", path, command.usage_string());
            break;
        case commands::Result::subcommands:
			log_subcommands(logger, path, command.subcommands());
            break;
        case commands::Result::unavailable:
			LOG_CONSOLE_ERROR_ASYNC(logger, R"(Command "{}" is currently unavailable.)", path);
            break;
        default:
			LOG_CONSOLE_ERROR_ASYNC(logger, R"(An error occurred while executing "{}")", path);
            break;
    }
}

commands::ArgumentValue convert_type(commands::ArgumentType type, std::string_view token) {
	switch(type) {
		case commands::ArgumentType::at_char:
			return boost::lexical_cast<char>(token);
			break;
		case commands::ArgumentType::at_string:
			return boost::lexical_cast<char>(token);
			break;
		case commands::ArgumentType::at_uint8:
			return boost::lexical_cast<std::uint8_t>(token);
			break;
		case commands::ArgumentType::at_uint16:
			return boost::lexical_cast<std::uint16_t>(token);
			break;
		case commands::ArgumentType::at_uint32:
			return boost::lexical_cast<std::uint32_t>(token);
			break;
		case commands::ArgumentType::at_uint64:
			return boost::lexical_cast<std::uint64_t>(token);
			break;
		case commands::ArgumentType::at_int8:
			return boost::lexical_cast<std::int8_t>(token);
			break;
		case commands::ArgumentType::at_int16:
			return boost::lexical_cast<std::int16_t>(token);
			break;
		case commands::ArgumentType::at_int32:
			return boost::lexical_cast<std::int32_t>(token);
			break;
		case commands::ArgumentType::at_int64:
			return boost::lexical_cast<std::int64_t>(token);
			break;
		case commands::ArgumentType::at_float:
			return boost::lexical_cast<float>(token);
			break;
		case commands::ArgumentType::at_double:
			return boost::lexical_cast<double>(token);
			break;
		default:
			throw exception("Unhandled argument type");
	}
}

void execute_command(std::string_view input, const commands::Registry& registry, log::Logger& logger) try {
	const auto tokens = registry.parse_input(input);
	const auto search = registry.search(tokens);

	if(!search.command) {
		LOG_CONSOLE_ERROR_ASYNC(logger, R"(Command "{}" not found)", tokens.front());
		return;
	}

	auto arguments = std::span(tokens).subspan(search.depth);
	std::vector<commands::ArgumentValue> arg_values;
	auto command_args = search.command->args();

	for(auto [expected, token] : std::views::zip(command_args, tokens)) {
		arg_values.emplace_back(convert_type(expected.type, token));
	}

	if(auto result = search.command->execute(arg_values); result != commands::Result::success) {
		const auto& path = commands::path_fragment(tokens, search.depth);
		handle_command_result(result, path, *search.command, logger);
	}
} catch(const commands::parse_error& e) {
	LOG_CONSOLE_ERROR_ASYNC(logger, R"(Error parsing command arguments, "{}")", e.what());
} catch(const std::exception& e) {
	LOG_CONSOLE_ERROR_ASYNC(logger, R"(Error during command execution, "{}")", e.what());
}

std::shared_ptr<commands::Command> register_help_command(commands::Registry& registry, log::Logger& logger) {
	auto handler = [&](const auto&) {
		LOG_CONSOLE_ASYNC(logger, "To display a list of available commands, press tab for autocompletion");
	};

	return registry.register_command("help")
		->description("Display console command usage information")
		->handler(handler);
}

void register_command_handlers(commands::Registry& registry, log::Logger& logger) {
#ifdef _WIN32
	auto sinks = logger.fetch_sink("CommandSink");

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