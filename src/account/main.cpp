/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include <logger/Logger.h>
#include <banner/Banner.h>
#include <commands/Commands.h>
#include <thread/Utility.h>
#include <shared/utility/CommandHelpers.h>
#include <shared/utility/LogConfig.h>
#include <shared/utility/Utility.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <cstdlib>

using namespace ember;
namespace opts = boost::program_options;

opts::variables_map parse_arguments(int argc, const char* argv[]);
std::shared_ptr<commands::Command> init_registry(const opts::variables_map& args, log::Logger& logger);
int run(const opts::variables_map& args, log::Logger& logger, commands::Command& registry);

/*
 * We want to do the minimum amount of work required to get 
 * logging facilities and crash handlers up and running in main.
 *
 * Exceptions that aren't derived from std::exception are
 * left to the crash handler since we can't get useful information
 * from them.
 */
int main(int argc, const char* argv[]) try {
	print_banner(account::app_name);
	utility::set_window_title(account::app_name);

	const auto args = parse_arguments(argc, argv);

	log::Logger logger;
	utility::configure_logger(logger, args);
	log::global_logger(logger);
	SLOG_INFO(logger, "Logger configured successfully");

	auto registry = init_registry(args, logger);

	if(!registry) {
		return EXIT_FAILURE;
	}

	const auto ret = run(args, logger, *registry);
	SLOG_INFO(logger, "{} terminated (returned '{}')", account::app_name, ret);
	return ret;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

int run(const opts::variables_map& args, log::Logger& logger, commands::Command& registry) {
	boost::asio::io_context ioc;
	boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

	account::Service service(logger, registry);

	signals.async_wait([&](auto ec, auto signal) {
		if(ec) {
			return;
		}

		SLOG_DEBUG(logger, "Received signal {}({})", utility::sig_str(signal), signal);
		service.stop();
	});

	std::jthread worker([&]() {
		thread::set_name("Signal handler");
		ioc.run();
	});

	thread::set_name("Service runner");
	const auto result = service.run(args);

	signals.cancel();
	return result;
}

std::shared_ptr<commands::Command> init_registry(const opts::variables_map& args, log::Logger& logger) try {
	const auto suggestions = args["console_log.suggestions"].as<bool>();
	auto registry = commands::create("root");
	utility::register_command_handlers(*registry, logger, suggestions);
	utility::register_common_commands(*registry, logger);
	return registry;
} catch(const std::exception& e) {
	SLOG_FATAL(logger, e.what());
	return nullptr;
}

opts::variables_map parse_arguments(const int argc, const char* argv[]) {
	// Command-line options
	opts::options_description cmdline_opts("Generic options");
	cmdline_opts.add_options()
		("help,h", "Displays a list of available options")
		("config,c", opts::value<std::string>()->default_value("account.conf"),
			"Path to the configuration file");

	opts::positional_options_description pos;
	pos.add("config", 1);

	// Config file options
	opts::options_description opts("Account configuration options");
	opts.add(account::Service::options());
	opts.add_options()
		("console_log.enable_input", opts::value<bool>()->required())
		("console_log.verbosity", opts::value<log::Severity>()->required())
		("console_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("console_log.colours", opts::bool_switch()->required())
		("console_log.suggestions", opts::value<bool>()->required())
		("remote_log.verbosity", opts::value<log::Severity>()->required())
		("remote_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("remote_log.service_name", opts::value<std::string>()->required())
		("remote_log.host", opts::value<std::string>()->required())
		("remote_log.port", opts::value<std::uint16_t>()->required())
		("file_log.verbosity", opts::value<log::Severity>()->required())
		("file_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("file_log.path", opts::value<std::string>()->default_value("account.log"))
		("file_log.timestamp_format", opts::value<std::string>())
		("file_log.mode", opts::value<std::string>()->required())
		("file_log.size_rotate", opts::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", opts::bool_switch()->required())
		("file_log.log_timestamp", opts::bool_switch()->required())
		("file_log.log_severity", opts::bool_switch()->required());

	opts::variables_map options;
	opts::store(opts::command_line_parser(argc, argv).positional(pos).options(cmdline_opts).run(), options);
	opts::notify(options);

	if(options.count("help")) {
		std::cout << cmdline_opts;
		std::exit(EXIT_SUCCESS);
	}

	const std::string& config_path = options["config"].as<std::string>();
	std::ifstream ifs(config_path);

	if(!ifs) {
		throw std::invalid_argument("Unable to open configuration file: " + config_path);
	}

	opts::store(opts::parse_config_file(ifs, opts), options);
	opts::notify(options);

	return options;
}