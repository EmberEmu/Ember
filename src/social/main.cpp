/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */


#include "FilterTypes.h"
#include "MonitorCallbacks.h"
#include <logger/Logger.h>
#include <banner/Banner.h>
#include <shared/utility/LogConfig.h>
#include <shared/utility/polyfill/print>
#include <shared/metrics/MetricsImpl.h>
#include <shared/metrics/Monitor.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/version.hpp>
#include <boost/program_options.hpp>
#include <boost/range/adaptor/map.hpp>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

using namespace ember;
namespace opts = boost::program_options;

void print_lib_versions(log::Logger& logger);
int launch(const opts::variables_map& args, log::Logger& logger);
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
	print_banner("Social Daemon");

	const auto args = parse_arguments(argc, argv);

	log::Logger logger;
	utility::configure_logger(logger, args);
	log::global_logger(logger);
	LOG_INFO(logger) << "Logger configured successfully" << LOG_SYNC;

	print_lib_versions(logger);
	const auto ret = launch(args, logger);
	LOG_INFO(logger) << "Social daemon terminated" << LOG_SYNC;
	return ret;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

int launch(const opts::variables_map& args, log::Logger& logger) try {
#ifdef DEBUG_NO_THREADS
	LOG_WARN_SYNC(logger, "Compiled with DEBUG_NO_THREADS!");
#endif

	boost::asio::io_context service;

	// Start metrics service
	auto metrics = std::make_unique<ember::Metrics>();

	if(args["metrics.enabled"].as<bool>()) {
		LOG_INFO(logger) << "Starting metrics service..." << LOG_SYNC;
		metrics = std::make_unique<ember::MetricsImpl>(
			service, args["metrics.statsd_host"].as<std::string>(),
			args["metrics.statsd_port"].as<std::uint16_t>()
		);
	}

	// Start monitoring service
	std::unique_ptr<ember::Monitor> monitor;

	if(args["monitor.enabled"].as<bool>()) {
		LOG_INFO(logger) << "Starting monitoring service..." << LOG_SYNC;

		monitor = std::make_unique<ember::Monitor>(
			service, args["monitor.interface"].as<std::string>(),
			args["monitor.port"].as<std::uint16_t>()
		);
	}

	boost::asio::dispatch(service, [&]() {
		LOG_INFO(logger) << "Social daemon started successfully" << LOG_SYNC;
	});

	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
	return EXIT_FAILURE;
}

opts::variables_map parse_arguments(const int argc, const char* argv[]) {
	//Command-line options
	opts::options_description cmdline_opts("Generic options");
	cmdline_opts.add_options()
		("help,h", "Displays a list of available options")
		("database.config_path,d", opts::value<std::string>(),
			"Path to the database configuration file")
		("config,c", opts::value<std::string>()->default_value("social.conf"),
			"Path to the configuration file");

	opts::positional_options_description pos; 
	pos.add("config", 1);

	//Config file options
	opts::options_description config_opts("Login configuration options");
	config_opts.add_options()
		("social.server_group", opts::value<unsigned int>()->required())
		("spark.address", opts::value<std::string>()->required())
		("spark.address", opts::value<std::string>()->required())
		("spark.port", opts::value<std::uint16_t>()->required())
		("network.interface", opts::value<std::string>()->required())
		("network.port", opts::value<std::uint16_t>()->required())
		("network.tcp_no_delay", opts::value<bool>()->required())
		("console_log.enable_input", opts::value<bool>()->required())
		("console_log.verbosity", opts::value<log::Severity>()->required())
		("console_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("console_log.colours", opts::value<bool>()->required())
		("remote_log.verbosity", opts::value<log::Severity>()->required())
		("remote_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("remote_log.service_name", opts::value<std::string>()->required())
		("remote_log.host", opts::value<std::string>()->required())
		("remote_log.port", opts::value<std::uint16_t>()->required())
		("file_log.verbosity", opts::value<log::Severity>()->required())
		("file_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("file_log.path", opts::value<std::string>()->default_value("social.log"))
		("file_log.timestamp_format", opts::value<std::string>())
		("file_log.mode", opts::value<std::string>()->required())
		("file_log.size_rotate", opts::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", opts::value<bool>()->required())
		("file_log.log_timestamp", opts::value<bool>()->required())
		("file_log.log_severity", opts::value<bool>()->required())
		("database.config_path", opts::value<std::string>()->required())
		("database.min_connections", opts::value<unsigned short>()->required())
		("database.max_connections", opts::value<unsigned short>()->required())
		("metrics.enabled", opts::value<bool>()->required())
		("metrics.statsd_host", opts::value<std::string>()->required())
		("metrics.statsd_port", opts::value<std::uint16_t>()->required())
		("monitor.enabled", opts::value<bool>()->required())
		("monitor.interface", opts::value<std::string>()->required())
		("monitor.port", opts::value<std::uint16_t>()->required());

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


void print_lib_versions(log::Logger& logger) {
	LOG_DEBUG(logger)
		<< "Compiled with library versions: " << "\n"
		<< " - Boost " << BOOST_VERSION / 100000 << "."
		<< BOOST_VERSION / 100 % 1000 << "."
		<< BOOST_VERSION % 100 << LOG_SYNC;
}