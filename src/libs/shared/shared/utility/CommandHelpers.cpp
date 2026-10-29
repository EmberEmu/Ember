/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CommandHelpers.h"
#include <banner/Version.h>
#include <commands/Commands.h>
#include <logger/CommandSink.h>
#include <logger/posix/CommandSink.h>
#include <logger/Logger.h>
#include <shared/utility/DurationString.h>
#include <shared/utility/Truncate.h>
#include <bprinter/table_printer.h>
#include <boost/lexical_cast.hpp>
#include <chrono>
#include <filesystem>
#include <ranges>
#include <span>
#include <sstream>
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

bool convert_bool(const std::string_view token) {
	if(token == "on" || token == "enable" || token == "1" || token == "true") {
		return true;
	} else if(token == "off" || token == "disable" || token == "0" || token == "false") {
		return false;
	}

	throw std::runtime_error("Unable to convert bool argument");
}

// ideally this should be replaced by free functions, but it'll do for now
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
	} else if(info == typeid(utility::DurationString)) {
		return DurationString(std::string(token));
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

void print_themes(std::span<const log::posix::ThemeView> themes,
                  std::span<const std::string> files,
                  log::Logger& logger) {
	constexpr auto max_name_len = 20u; // todo, size_t literals on bump
	constexpr auto max_desc_len = 80u;

	std::stringstream stream;
	stream << "Available themes:" << '\n';

	bprinter::TablePrinter printer(&stream);

	printer.AddColumn("Name", max_name_len);
	printer.AddColumn("Description", max_desc_len);
	printer.PrintHeader();

	for(const auto& theme : themes) {
		utility::write_pretty_truncate(printer, theme.name, max_name_len);
		utility::write_pretty_truncate(printer, theme.desc, max_desc_len);
	}

	for(const auto& file : files) {
		utility::write_pretty_truncate(printer, file, max_name_len);
		utility::write_pretty_truncate(printer, "Custom theme file", max_desc_len);
	}

	printer.PrintFooter();
	LOG_CONSOLE(logger, stream.view());
}

std::vector<std::string> theme_file_list_relative(const std::filesystem::path& path) {
	std::vector<std::string> files;

	if(std::filesystem::is_directory(path)) {
		for(auto& file : std::filesystem::directory_iterator(path)) {
			if(file.path().string().ends_with(".theme.json")) {
				files.emplace_back(file.path().filename().string());
			}
		}
	}

	return files;
}

void handle_theme_list_command(const std::filesystem::path& path, log::Logger& logger) {
	const auto themes = log::posix::builtin_themes();
	const auto files = theme_file_list_relative(path);

	if(themes.empty() && files.empty()) {
		LOG_CONSOLE(logger, "No themes available");
	} else {
		print_themes(themes, files, logger);
	}
}

commands::Suggestions handle_theme_set_autocomplete(std::span<const std::string> tokens,
                                                    const std::filesystem::path& path) {
	if(tokens.size() > 1) {
		return {};
	}

	commands::Suggestions results;

	// built-in themes
	for(const auto& theme : log::posix::builtin_themes()) {
		if(tokens.empty() || theme.name.starts_with(tokens.front())) {
			results.records.emplace_back(
				std::string(theme.name),
				std::string(theme.desc)
			);
		}
	}

	// custom theme files
	for(const auto& file : theme_file_list_relative(path)) {
		if(tokens.empty() || file.starts_with(tokens.front())) {
			results.records.emplace_back(
				file, "Custom theme file"
			);
		}
	}

	// todo, move out of impl or another approach?
	results.substring = commands::impl::longest_prefix(results.records);
	return results;
}

void handle_theme_set_command(const commands::Arguments& arguments,
                              const std::filesystem::path& path,
                              log::Logger& logger) try {
	const auto sink = logger.fetch_sink(log::posix::CommandSink::name());

	if(!sink) {
		throw std::runtime_error("command sink not found");
	}
	
	auto posix_sink = std::dynamic_pointer_cast<log::posix::CommandSink>(sink);

	if(!posix_sink) {
		throw std::runtime_error("incompatible sink type found");
	}

	const auto name = arguments["name"].as<std::string>();

	log::posix::Theme theme;
	
	if(log::posix::has_builtin_theme(name)) {
		theme = log::posix::builtin_theme(name);
	} else {
		const auto absolute = path / name;
		theme = log::posix::load_theme(absolute);
	}

	const std::string theme_name(theme.name);

	posix_sink->set_theme(std::move(theme));
	posix_sink->clear_console();

	LOG_CONSOLE(logger, "Theme set to {}", theme_name);
} catch(const std::exception& e) {
	LOG_CONERR(logger, "Error setting theme: {}", e.what());
}

void handle_cls_command(log::Logger& logger) {
	const auto result = logger.fetch_unique(log::CommandSink::type());

	if(!result) {
		SLOG_ERROR(logger, "Could not locate a command sink, cannot execute cls");
		return;
	}

	auto& sink = static_cast<log::CommandSink&>(*result);
	sink.clear_console();
}

// command handler keeps 'root' alive so shared_ptr is not strictly necessary
// but it also doesn't hurt since this not performance sensitive
void register_common_commands(std::shared_ptr<commands::Command> root, log::Logger& logger) {
	root->insert("help")
		->description("Display console command usage information")
		->argument<std::string>("command", commands::optional)
		->handler([&, root](const auto& arguments) {
			handle_help_command(arguments, *root, logger);
		});

	root->insert("version")
		->description("Display version and build information")
		->handler([&](const auto&) {
			LOG_CONSOLE(
				logger, "{}, {} ({}, {})", build::version, build::git_hash, build::date, build::time
			);
		});

	root->insert("cls")
		->description("Clears the console")
		->handler([&](const auto&) {
			handle_cls_command(logger);
		});
}

void register_theme_commands(std::shared_ptr<commands::Command> root, log::Logger& logger, std::filesystem::path theme_path) {
	auto theme_cmd = root->insert("theme")
		->description("Console theme options");

	theme_cmd->insert("set")
		->description("Change the console's current theme")
		->argument<std::string>("name")
		->handler([&, theme_path](const auto& arguments) {
			handle_theme_set_command(arguments, theme_path, logger);
		})
		->autocomplete([theme_path](const auto& tokens) {
			return handle_theme_set_autocomplete(tokens, theme_path);
		});
	
	theme_cmd->insert("list")
		->description("Display available console themes")
		->handler([&, theme_path](const auto&) {
			handle_theme_list_command(theme_path, logger);
		});
}

void register_command_handlers(std::shared_ptr<commands::Command> root, log::Logger& logger, const bool suggestions) {
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