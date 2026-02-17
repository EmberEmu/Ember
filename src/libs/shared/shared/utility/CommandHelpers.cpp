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
#include <ranges>
#include <span>
#include <string>

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

void execute_command(std::string_view input, const commands::Registry& registry, log::Logger& logger) try {
	const auto tokens = registry.parse_input(input);
	const auto search = registry.search(tokens);

	if(!search.command) {
		LOG_CONSOLE_ERROR_ASYNC(logger, R"(Command "{}" not found)", tokens.front());
		return;
	}

	const auto arguments = std::span(tokens).subspan(search.depth);

	if(auto result = search.command->execute(arguments); result != commands::Result::success) {
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