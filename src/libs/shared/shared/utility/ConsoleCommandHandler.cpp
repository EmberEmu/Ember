/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ConsoleCommandHandler.h"
#include <commands/Utility.h>
#include <logger/posix/CommandSink.h>
#include <logger/Logger.h>
#include <shared/utility/DurationString.h>
#include <chrono>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ember::utility {

void log_subcommands(const std::string& path, const commands::CommandsView& subcommands, log::Logger& logger) {
	LOG_CONSOLE(logger, R"(Available subcomands for "{}": )", path);

	for(auto& subcommand : subcommands | std::views::values) {
		LOG_CONSOLE(logger, "{} : {}", subcommand->name(), subcommand->description());
	}
}

void handle_command_result(commands::Result result,
                           const std::string& path,
                           const commands::Command& command,
                           log::Logger& logger) {
	using enum commands::Result;

    switch(result) {
		case missing_args:
			[[fallthrough]];
        case too_many_args:
			LOG_CONERR(logger, "Usage: {} {}", path, command.usage_string());
            break;
        case subcommands:
			log_subcommands(path, command.view(), logger);
            break;
        case unavailable:
			LOG_CONERR(logger, R"(Command "{}" is currently unavailable.)", path);
            break;
		case invalid_types:
			LOG_CONERR(logger, R"(Bad argument types when attempting to execute "{}")", path);
			break;
        default:
			LOG_CONERR(logger, R"(An unhandled error occurred while executing "{}")", path);
            break;
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

	// execute the command handler
	if(auto result = search.command->execute(arguments); result != commands::Result::success) {
		const auto& path = commands::path_fragment(tokens, search.depth);
		handle_command_result(result, path, *search.command, logger);
	}
} catch(const commands::parse_error& e) {
	LOG_CONERR(logger, R"(Error parsing arguments, "{}")", e.what());
} catch(const std::exception& e) {
	LOG_CONERR(logger, R"(Error during command execution, "{}")", e.what());
}

void register_console_command_handler(std::shared_ptr<commands::Command> root, log::Logger& logger, const bool suggestions) {
	auto result = logger.fetch_unique(log::CommandSink::type());

	if(!result) {
		SLOG_INFO(logger, "Console commands disabled, no suitable logging sink found");
		return;
	}

	auto& sink = static_cast<log::CommandSink&>(*result);

	sink.register_autocomplete([root](auto cmd) {
		return root->autocomplete(cmd);
	});

	sink.register_handler([&, root](auto input) {
		execute_command(input, *root, logger);
	});

	if(suggestions) {
		sink.register_suggestion([root](auto cmd) {
			return root->autocomplete(cmd).substring;
		});
	}
}

} // utility, ember