/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include <logger/Logger.h>
#include <banner/Banner.h>
#include <commands/Registry.h>
#include <thread/Utility.h>
#include <shared/utility/CommandHelpers.h>
#include <shared/utility/LogConfig.h>
#include <shared/utility/Utility.h>
#include <shared/utility/cstring_view.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdlib>

using namespace ember;
namespace opts = boost::program_options;

int launch(const opts::variables_map& args, log::Logger& logger, commands::Registry& registry);
opts::variables_map parse_arguments(int argc, const char* argv[]);

/*
 * We want to do the minimum amount of work required to get 
 * logging facilities and crash handlers up and running in main.
 *
 * Exceptions that aren't derived from std::exception are
 * left to the crash handler since we can't get useful information
 * from them.
 */
int main(int argc, const char* argv[]) try {
	thread::set_name("Main");
	print_banner(world::app_name);
	utility::set_window_title(world::app_name);

	const auto args = parse_arguments(argc, argv);

	log::Logger logger;
	utility::configure_logger(logger, args);
	log::global_logger(logger);

	LOG_INFO_SYNC(logger, "Logger configured successfully");

	LOG_DEBUG_SYNC(logger, "Registering command handlers...");
	commands::Registry registry;
	utility::register_command_handlers(registry, logger);
	utility::register_shared_commands(registry, logger);

	const auto ret = launch(args, logger, registry);
	LOG_INFO_SYNC(logger, "{} terminated (returned '{}')", world::app_name, ret);
	return ret;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

int launch(const opts::variables_map& args, log::Logger& logger, commands::Registry& registry) try {
	world::Service service(logger, registry);
	return service.run(args);
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, "{}", e.what());
	return EXIT_FAILURE;
}

opts::variables_map parse_arguments(const int argc, const char* argv[]) {
	// Command-line options
	opts::options_description cmdline_opts("Generic options");
	cmdline_opts.add_options()
		("help,h", "Displays a list of available options")
		("database.config_path,d", opts::value<std::string>(),
			"Path to the database configuration file")
		("config,c", opts::value<std::string>()->default_value("world.conf"),
			"Path to the configuration file");

	opts::positional_options_description pos;
	pos.add("config", 1);

	// Config file options
	opts::options_description config_opts("World configuration options");
	config_opts.add(world::Service::options());

	opts::variables_map options;
	opts::store(opts::command_line_parser(argc, argv).positional(pos).options(cmdline_opts).run(), options);
	opts::notify(options);

	if(options.count("help")) {
		std::cout << cmdline_opts;
		std::exit(EXIT_SUCCESS);
	}

	const auto& config_path = options["config"].as<std::string>();
	std::ifstream ifs(config_path);

	if(!ifs) {
		throw std::invalid_argument("Unable to open configuration file: " + config_path);
	}

	opts::store(opts::parse_config_file(ifs, config_opts), options);
	opts::notify(options);

	return options;
}