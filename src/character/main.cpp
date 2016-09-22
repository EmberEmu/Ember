/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FilterTypes.h"
#include "CharacterHandler.h"
#include "Service.h"
#include <dbcreader/DBCReader.h>
#include <spark/Spark.h>
#include <conpool/ConnectionPool.h>
#include <conpool/Policies.h>
#include <conpool/drivers/AutoSelect.h>
#include <logger/Logging.h>
#include <shared/Banner.h>
#include <shared/database/daos/CharacterDAO.h>
#include <shared/threading/ThreadPool.h>
#include <shared/Version.h>
#include <shared/util/LogConfig.h>
#include <shared/util/PCREHelper.h>
#include <shared/util/Utility.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>

const std::string APP_NAME = "Character Daemon";

namespace ep = ember::connection_pool;
namespace po = boost::program_options;
namespace ba = boost::asio;
using namespace std::chrono_literals;

namespace ember {

void launch(const po::variables_map& args, log::Logger* logger);
unsigned int check_concurrency(log::Logger* logger); // todo, move
po::variables_map parse_arguments(int argc, const char* argv[]);
void pool_log_callback(ep::Severity, const std::string& message, log::Logger* logger);

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
	ember::print_banner(APP_NAME);
	ember::util::set_window_title(APP_NAME);

	const po::variables_map args = ember::parse_arguments(argc, argv);

	auto logger = ember::util::init_logging(args);
	ember::log::set_global_logger(logger.get());
	LOG_INFO(logger) << "Logger configured successfully" << LOG_SYNC;

	ember::launch(args, logger.get());
	LOG_INFO(logger) << APP_NAME << " terminated" << LOG_SYNC;
} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}

namespace ember {

void launch(const po::variables_map& args, log::Logger* logger) try {
#ifdef DEBUG_NO_THREADS
	LOG_WARN(logger) << "Compiled with DEBUG_NO_THREADS!" << LOG_SYNC;
#endif

	LOG_INFO(logger) << "Loading DBC data..." << LOG_SYNC;
	dbc::DiskLoader loader(args["dbc.path"].as<std::string>(), [&](auto message) {
		LOG_DEBUG(logger) << message << LOG_SYNC;
	});

	auto dbc_store = loader.load({
		"ChrClasses", "ChrRaces", "CharBaseInfo", "NamesProfanity", "NamesReserved", "CharSections",
		"CharacterFacialHairStyles", "CharStartBase", "CharStartSpells", "CharStartTalents",
		"CharStartZones", "CharStartOutfit", "AreaTable", "FactionTemplate", "FactionGroup"
	});

	LOG_INFO(logger) << "Resolving DBC references..." << LOG_SYNC;
	dbc::link(dbc_store);

	LOG_INFO_GLOB << "Compiling DBC regular expressions..." << LOG_ASYNC;
	std::vector<ember::util::pcre::Result> profanity, reserved;

	for(auto& i : dbc_store.names_profanity.values()) {
		profanity.emplace_back(ember::util::pcre::utf8_jit_compile(i.name));
	}

	for(auto& i : dbc_store.names_reserved.values()) {
		reserved.emplace_back(ember::util::pcre::utf8_jit_compile(i.name));
	}

	LOG_INFO(logger) << "Initialising database driver..." << LOG_SYNC;
	auto db_config_path = args["database.config_path"].as<std::string>();
	auto driver(ember::drivers::init_db_driver(db_config_path));

	LOG_INFO(logger) << "Initialising database connection pool..." << LOG_SYNC;
	auto min_conns = args["database.min_connections"].as<unsigned short>();
	auto max_conns = args["database.max_connections"].as<unsigned short>();
	auto concurrency = check_concurrency(logger);

	if(max_conns != concurrency) {
		LOG_WARN(logger) << "Max. database connection count may be non-optimal (use "
			<< concurrency << " to match logical core count)" << LOG_SYNC;
	}

	LOG_INFO(logger) << "Initialising database connection pool..." << LOG_SYNC;
	ep::Pool<decltype(driver), ep::CheckinClean, ep::ExponentialGrowth> pool(driver, min_conns, max_conns, 30s);
	pool.logging_callback(std::bind(pool_log_callback, std::placeholders::_1, std::placeholders::_2, logger));

	LOG_INFO(logger) << "Initialising DAOs..." << LOG_SYNC;
	auto character_dao = ember::dal::character_dao(pool);

	std::locale temp;

	LOG_INFO(logger) << "Starting Spark service..." << LOG_SYNC;
	auto s_address = args["spark.address"].as<std::string>();
	auto s_port = args["spark.port"].as<std::uint16_t>();
	auto mcast_group = args["spark.multicast_group"].as<std::string>();
	auto mcast_iface = args["spark.multicast_interface"].as<std::string>();
	auto mcast_port = args["spark.multicast_port"].as<std::uint16_t>();
	auto spark_filter = log::Filter(ember::FilterType::LF_SPARK);

	boost::asio::io_service service;
	boost::asio::signal_set signals(service, SIGINT, SIGTERM);

	ThreadPool thread_pool(concurrency);
	ember::CharacterHandler handler(profanity, reserved, dbc_store, *character_dao, thread_pool, temp, logger);

	spark::Service spark("character", service, s_address, s_port, logger, spark_filter);
	spark::ServiceDiscovery discovery(service, s_address, s_port, mcast_iface, mcast_group,
	                               mcast_port, logger, spark_filter);

	ember::Service char_service(*character_dao, handler, spark, discovery, logger);
	
	signals.async_wait([&](const boost::system::error_code& error, int signal) {
		LOG_INFO(logger) << APP_NAME << " shutting down..." << LOG_SYNC;
		discovery.shutdown();
		spark.shutdown();
		thread_pool.shutdown();
		pool.close();
	});

	service.dispatch([&, logger]() {
		LOG_INFO(logger) << APP_NAME << " started successfully" << LOG_SYNC;
	});

	service.run();
} catch(std::exception& e) {
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	//Command-line options
	po::options_description cmdline_opts("Generic options");
	cmdline_opts.add_options()
		("help", "Displays a list of available options")
		("config,c", po::value<std::string>()->default_value("character.conf"),
			"Path to the configuration file");

	po::positional_options_description pos; 
	pos.add("config", 1);

	//Config file options
	po::options_description config_opts("Character service configuration options");
	config_opts.add_options()
		("dbc.path", po::value<std::string>()->required())
		("spark.address", po::value<std::string>()->required())
		("spark.port", po::value<std::uint16_t>()->required())
		("spark.multicast_interface", po::value<std::string>()->required())
		("spark.multicast_group", po::value<std::string>()->required())
		("spark.multicast_port", po::value<std::uint16_t>()->required())
		("console_log.verbosity", po::value<std::string>()->required())
		("console_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("console_log.colours", po::bool_switch()->required())
		("remote_log.verbosity", po::value<std::string>()->required())
		("remote_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("remote_log.service_name", po::value<std::string>()->required())
		("remote_log.host", po::value<std::string>()->required())
		("remote_log.port", po::value<std::uint16_t>()->required())
		("file_log.verbosity", po::value<std::string>()->required())
		("file_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("file_log.path", po::value<std::string>()->default_value("character.log"))
		("file_log.timestamp_format", po::value<std::string>())
		("file_log.mode", po::value<std::string>()->required())
		("file_log.size_rotate", po::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", po::bool_switch()->required())
		("file_log.log_timestamp", po::bool_switch()->required())
		("file_log.log_severity", po::bool_switch()->required())
		("database.config_path", po::value<std::string>()->required())
		("database.min_connections", po::value<unsigned short>()->required())
		("database.max_connections", po::value<unsigned short>()->required())
		("metrics.enabled", po::bool_switch()->required())
		("metrics.statsd_host", po::value<std::string>()->required())
		("metrics.statsd_port", po::value<std::uint16_t>()->required())
		("monitor.enabled", po::bool_switch()->required())
		("monitor.interface", po::value<std::string>()->required())
		("monitor.port", po::value<std::uint16_t>()->required());

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

void pool_log_callback(ep::Severity severity, const std::string& message, log::Logger* logger) {
	using ember::LF_DB_CONN_POOL;

	switch(severity) {
		case(ep::Severity::DEBUG) :
			LOG_DEBUG_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
			break;
		case(ep::Severity::INFO) :
			LOG_INFO_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
			break;
		case(ep::Severity::WARN) :
			LOG_WARN_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
			break;
		case(ep::Severity::ERROR) :
			LOG_ERROR_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
			break;
		case(ep::Severity::FATAL) :
			LOG_FATAL_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
			break;
		default:
			LOG_ERROR_FILTER(logger, LF_DB_CONN_POOL) << "Unhandled pool log callback severity" << LOG_ASYNC;
			LOG_ERROR_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
	}
}

/*
 * The concurrency level returned is usually the number of logical cores
 * in the machine but the standard doesn't guarantee that it won't be zero.
 * In that case, we just set the minimum concurrency level to two.
 */
unsigned int check_concurrency(log::Logger* logger) {
	unsigned int concurrency = std::thread::hardware_concurrency();

	if(!concurrency) {
		concurrency = 2;
		LOG_WARN(logger) << "Unable to determine concurrency level" << LOG_SYNC;
	}

#ifdef DEBUG_NO_THREADS // todo, this shouldn't be needed
	return 0;
#else
	return concurrency;
#endif
}

} // ember