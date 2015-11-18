/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LogFilters.h"
#include <conpool/ConnectionPool.h>
#include <conpool/Policies.h>
#include <conpool/drivers/AutoSelect.h>
#include <logger/Logging.h>
#include <shared/Banner.h>
#include <shared/Version.h>
#include <shared/util/LogConfig.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>

namespace el = ember::log;
namespace ef = ember::filter;
namespace ep = ember::connection_pool;
namespace po = boost::program_options;
namespace ba = boost::asio;

void launch(const po::variables_map& args, el::Logger* logger);
po::variables_map parse_arguments(int argc, const char* argv[]);
ember::drivers::MySQL init_db_driver(const po::variables_map& args);
int fetch_realm(unsigned int id);

int fetch_realm(unsigned int id) {
	return 0;
}

/*
 * We want to do the minimum amount of work required to get 
 * logging facilities and crash handlers up and running in main.
 *
 * Exceptions that aren't derived from std::exception are
 * left to the crash handler since we can't get useful information
 * from them.
 */
int main(int argc, const char* argv[]) try {
	ember::print_banner("Realm Gateway");

	const po::variables_map args = parse_arguments(argc, argv);

	auto logger = ember::util::init_logging(args);
	el::set_global_logger(logger.get());
	LOG_INFO(logger) << "Logger configured successfully" << LOG_SYNC;

	launch(args, logger.get());
} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}

void launch(const po::variables_map& args, el::Logger* logger) try {
	LOG_INFO(logger) << "Retrieving realm configuration..."<< LOG_SYNC;

	auto realm_id = args["realm.id"].as<unsigned short>();
	auto realm = fetch_realm(realm_id);

	auto max_slots = args["realm.max-slots"].as<unsigned int>();
	auto reserved_slots = args["realm.reserved-slots"].as<unsigned int>();
	
} catch(std::exception& e) {
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	//Command-line options
	po::options_description cmdline_opts("Generic options");
	cmdline_opts.add_options()
		("help", "Displays a list of available options")
		("config,c", po::value<std::string>()->default_value("gateway.conf"),
			"Path to the configuration file");

	po::positional_options_description pos; 
	pos.add("config", 1);

	//Config file options
	po::options_description config_opts("Realm gateway configuration options");
	config_opts.add_options()
		("realm.id", po::value<unsigned int>()->required())
		("realm.max-slots", po::value<unsigned int>()->required())
		("realm.reserved-slots", po::value<unsigned int>()->required())
		("network.interface", po::value<unsigned int>()->required())
		("network.interface", po::value<std::string>()->required())
		("network.port", po::value<unsigned short>()->required())
		("console_log.verbosity", po::value<std::string>()->required())
		("remote_log.filter-mask", po::value<unsigned int>()->required())
		("remote_log.verbosity", po::value<std::string>()->required())
		("remote_log.service_name", po::value<std::string>()->required())
		("remote_log.host", po::value<std::string>()->required())
		("remote_log.port", po::value<unsigned short>()->required())
		("file_log.verbosity", po::value<std::string>()->required())
		("file_log.path", po::value<std::string>()->default_value("gateway.log"))
		("file_log.timestamp_format", po::value<std::string>())
		("file_log.mode", po::value<std::string>()->required())
		("file_log.size_rotate", po::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", po::bool_switch()->required())
		("file_log.log_timestamp", po::bool_switch()->required())
		("file_log.log_severity", po::bool_switch()->required())
		("database.username", po::value<std::string>()->required())
		("database.password", po::value<std::string>()->default_value(""))
		("database.database", po::value<std::string>()->required())
		("database.host", po::value<std::string>()->required())
		("database.port", po::value<unsigned short>()->required())
		("database.min_connections", po::value<unsigned short>()->required())
		("database.max_connections", po::value<unsigned short>()->required());

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).positional(pos).options(cmdline_opts).run(), options);
	po::notify(options);

	if(options.count("help")) {
		std::cout << cmdline_opts << "\n";
		std::exit(0);
	}

	std::string config_path = options["config"].as<std::string>();
	std::ifstream ifs(config_path);

	if(!ifs) {
		std::string message("Unable to open configuration file: " + config_path);
		throw std::invalid_argument(message);
	}

	po::store(po::parse_config_file(ifs, config_opts), options);
	po::notify(options);

	return std::move(options);
}

ember::drivers::DriverType init_db_driver(const po::variables_map& args) {
	auto user = args["database.username"].as<std::string>();
	auto pass = args["database.password"].as<std::string>();
	auto host = args["database.host"].as<std::string>();
	auto port = args["database.port"].as<unsigned short>();
	auto db   = args["database.database"].as<std::string>();
	return {user, pass, host, port, db};
}