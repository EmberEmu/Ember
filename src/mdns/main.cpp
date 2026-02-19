/*
 * Copyright (c) 2021 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include <logger/Logger.h>
#include <banner/Banner.h>
#include <banner/Banner.h>
#include <shared/threading/Utility.h>
#include <shared/utility/LogConfig.h>
#include <shared/utility/Utility.h>
#include <shared/utility/cstring_view.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <fstream>
#include <iostream>
#include <thread>

using namespace ember;
namespace po = boost::program_options;

po::variables_map parse_arguments(int argc, const char* argv[]);
int run(const po::variables_map& args, log::Logger& logger);

/*
 * We want to do the minimum amount of work required to get 
 * logging facilities and crash handlers up and running in main.
 *
 * Exceptions that aren't derived from std::exception are
 * left to the crash handler since we can't get useful information
 * from them.
 */
int main(int argc, const char* argv[]) try {
	print_banner(dns::APP_NAME);
	utility::set_window_title(dns::APP_NAME);

	const auto args = parse_arguments(argc, argv);

	log::Logger logger;
	utility::configure_logger(logger, args);
	log::global_logger(logger);
	LOG_INFO_SYNC(logger, "Logger configured successfully");

	const auto ret = run(args, logger);
	LOG_INFO_SYNC(logger, "{} terminated (return code: {})", dns::APP_NAME, ret);
	return ret;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

int run(const po::variables_map& args, log::Logger& logger) try {
	boost::asio::io_context io_ctx;
	boost::asio::signal_set signals(io_ctx, SIGINT, SIGTERM);

	dns::Service service(logger);

	signals.async_wait([&](auto ec, auto signal) {
		if(ec) {
			return;
		}

		LOG_DEBUG_SYNC(logger, "Received signal {}({})", utility::sig_str(signal), signal);
		signals.clear();
		service.stop();
	});

	std::jthread worker([&]() {
		thread::set_name("Signal handler");
		io_ctx.run_one();
	});

	const auto ret =  service.run(args);
	signals.cancel();
	return ret;
} catch(const std::exception& e) {
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
	return EXIT_FAILURE;
}

po::variables_map parse_arguments(const int argc, const char* argv[]) {
	// Command-line options
	po::options_description cmdline_opts("Generic options");
	cmdline_opts.add_options()
		("help", "Displays a list of available options")
		("config,c", po::value<std::string>()->default_value("mdns.conf"),
			 "Path to the configuration file");

	po::positional_options_description pos;
	pos.add("config", 1);

	// Config file options
	po::options_description config_opts("Multicast DNS configuration options");
	config_opts.add(dns::Service::options());

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).positional(pos).options(cmdline_opts).run(), options);
	po::notify(options);

	if(options.count("help")) {
		std::cout << cmdline_opts;
		std::exit(EXIT_SUCCESS);
	}

	const auto& config_path = options["config"].as<std::string>();
	std::ifstream ifs(config_path);

	if(!ifs) {
		throw std::invalid_argument("Unable to open configuration file: " + config_path);
	}

	po::store(po::parse_config_file(ifs, config_opts), options);
	po::notify(options);

	return options;
}