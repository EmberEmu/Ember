/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Server.h"
#include "MulticastSocket.h"
#include "RequestHandler.h"
#include <logger/Logging.h>
#include <shared/Banner.h>
#include <shared/Version.h>
#include <shared/util/LogConfig.h>
#include <shared/util/Utility.h>
#include <shared/threading/Utility.h>
#include <spark/Spark.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <cstddef>

constexpr ember::cstring_view APP_NAME { "MDNS-SD" };

namespace el = ember::log;
namespace po = boost::program_options;
using namespace std::chrono_literals;

namespace ember {

int launch(const po::variables_map& args, log::Logger* logger);
unsigned int check_concurrency(log::Logger* logger); // todo, move
po::variables_map parse_arguments(int argc, const char* argv[]);

} // ember

/*
 * We want to do the minimum amount of work required to get 
 * logging facilities and crash handlers up and running in main.
 *
 * Exceptions that aren't derived from std::exception are
 * left to the crash handler since we can't get useful information
 * from them.
 */
int main(int argc, const char* argv[]) try {
	ember::thread::set_name("Main");
	ember::print_banner(APP_NAME);
	ember::util::set_window_title(APP_NAME);

	const po::variables_map args = ember::parse_arguments(argc, argv);

	auto logger = ember::util::init_logging(args);
	ember::log::set_global_logger(logger.get());
	LOG_INFO(logger) << "Logger configured successfully" << LOG_SYNC;

	const auto ret = ember::launch(args, logger.get());
	LOG_INFO(logger) << APP_NAME << " terminated" << LOG_SYNC;
	return ret;
} catch(std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

namespace ember {

int launch(const po::variables_map& args, log::Logger* logger) try {
#ifdef DEBUG_NO_THREADS
	LOG_WARN(logger) << "Compiled with DEBUG_NO_THREADS!" << LOG_SYNC;
#endif

	boost::asio::io_context service(BOOST_ASIO_CONCURRENCY_HINT_UNSAFE_IO);
	boost::asio::signal_set signals(service, SIGINT, SIGTERM);

	const auto& iface = args["mdns.interface"].as<std::string>();
	const auto& group = args["mdns.group"].as<std::string>();
	const auto port = args["mdns.port"].as<std::uint16_t>();

	// start multicast DNS services
	auto socket = std::make_unique<dns::MulticastSocket>(service, iface, group, port);
	dns::Server server(std::move(socket), logger);

	const auto& spark_iface = args["spark.address"].as<std::string>();
	const auto spark_port = args["spark.port"].as<std::uint16_t>();

	// start Spark services
	spark::v2::Server spark(service, APP_NAME, spark_iface, spark_port, logger);
	dns::RequestHandler handler(logger);
	//context.register_service(&handler);
	
	signals.async_wait([&](const boost::system::error_code& error, int signal) {
		LOG_TRACE(logger) << __func__ << signal << LOG_SYNC;
		server.shutdown();
		spark.shutdown();
	});

	service.dispatch([logger]() {
		LOG_INFO(logger) << APP_NAME << " started successfully" << LOG_SYNC;
	});

	service.run();

	LOG_INFO(logger) << APP_NAME << " shutting down..." << LOG_SYNC;
	return EXIT_SUCCESS;
} catch(std::exception& e) {
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
	return EXIT_FAILURE;
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
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
	config_opts.add_options()
		("mdns.interface", po::value<std::string>()->required())
		("mdns.group", po::value<std::string>()->required())
		("mdns.port", po::value<std::uint16_t>()->default_value(5353))
		("spark.address", po::value<std::string>()->required())
		("spark.port", po::value<std::uint16_t>()->required())
		("console_log.verbosity", po::value<std::string>()->required())
		("console_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("console_log.colours", po::value<bool>()->required())
		("remote_log.verbosity", po::value<std::string>()->required())
		("remote_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("remote_log.service_name", po::value<std::string>()->required())
		("remote_log.host", po::value<std::string>()->required())
		("remote_log.port", po::value<std::uint16_t>()->required())
		("file_log.verbosity", po::value<std::string>()->required())
		("file_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("file_log.path", po::value<std::string>()->default_value("world.log"))
		("file_log.timestamp_format", po::value<std::string>())
		("file_log.mode", po::value<std::string>()->required())
		("file_log.size_rotate", po::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", po::value<bool>()->required())
		("file_log.log_timestamp", po::value<bool>()->required())
		("file_log.log_severity", po::value<bool>()->required())
		("metrics.enabled", po::value<bool>()->required())
		("metrics.statsd_host", po::value<std::string>()->required())
		("metrics.statsd_port", po::value<std::uint16_t>()->required());

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).positional(pos).options(cmdline_opts).run(), options);
	po::notify(options);

	if(options.count("help")) {
		std::cout << cmdline_opts << "\n";
		std::exit(0);
	}

	const auto& config_path = options["config"].as<std::string>();
	std::ifstream ifs(config_path);

	if(!ifs) {
		std::string message("Unable to open configuration file: " + config_path);
		throw std::invalid_argument(message);
	}

	po::store(po::parse_config_file(ifs, config_opts), options);
	po::notify(options);

	return options;
}

} // ember