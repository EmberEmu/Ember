/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FilterTypes.h"
#include "GameVersion.h"
#include "SessionBuilders.h"
#include "LoginHandlerBuilder.h"
#include "MonitorCallbacks.h"
#include "NetworkListener.h"
#include "Patcher.h"
#include "RealmList.h"
#include <logger/Logging.h>
#include <conpool/ConnectionPool.h>
#include <conpool/Policies.h>
#include <conpool/drivers/AutoSelect.h>
#include <shared/Banner.h>
#include <shared/Version.h>
#include <shared/util/LogConfig.h>
#include <shared/metrics/MetricsImpl.h>
#include <shared/metrics/Monitor.h>
#include <shared/threading/ThreadPool.h>
#include <shared/database/daos/IPBanDAO.h>
#include <shared/database/daos/RealmDAO.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/IPBanCache.h>
#include <botan/init.h>
#include <botan/version.h>
#include <boost/version.hpp>
#include <boost/program_options.hpp>
#include <boost/range/adaptor/map.hpp>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace el = ember::log;
namespace ep = ember::connection_pool;
namespace po = boost::program_options;
namespace ba = boost::asio;

void print_lib_versions(el::Logger* logger);
std::vector<ember::GameVersion> client_versions();
unsigned int check_concurrency(el::Logger* logger);
void launch(const po::variables_map& args, el::Logger* logger);
po::variables_map parse_arguments(int argc, const char* argv[]);
std::unique_ptr<ember::log::Logger> init_logging(const po::variables_map& args);
void pool_log_callback(ep::Severity, const std::string& message, el::Logger* logger);

/*
 * We want to do the minimum amount of work required to get 
 * logging facilities and crash handlers up and running in main.
 *
 * Exceptions that aren't derived from std::exception are
 * left to the crash handler since we can't get useful information
 * from them.
 */
int main(int argc, const char* argv[]) try {
	ember::print_banner("Login Daemon");
	const po::variables_map args = parse_arguments(argc, argv);

	auto logger = ember::util::init_logging(args);
	el::set_global_logger(logger.get());
	LOG_INFO(logger) << "Logger configured successfully" << LOG_SYNC;

	print_lib_versions(logger.get());
	launch(args, logger.get());
	LOG_INFO(logger) << "Login daemon terminated" << LOG_SYNC;
} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}

void launch(const po::variables_map& args, el::Logger* logger) try {
#ifdef DEBUG_NO_THREADS
	LOG_WARN(logger) << "Compiled with DEBUG_NO_THREADS!" << LOG_SYNC;
#endif

	LOG_INFO(logger) << "Initialialising Botan..." << LOG_SYNC;
	Botan::LibraryInitializer init("thread_safe");

	unsigned int concurrency = check_concurrency(logger);

	LOG_INFO(logger) << "Initialising database driver..."<< LOG_SYNC;
	auto db_config_path = args["database.config_path"].as<std::string>();
	auto driver(ember::drivers::init_db_driver(db_config_path));
	auto min_conns = args["database.min_connections"].as<unsigned short>();
	auto max_conns = args["database.max_connections"].as<unsigned short>();

	LOG_INFO(logger) << "Initialising database connection pool..."<< LOG_SYNC;

	if(max_conns != concurrency) {
		LOG_WARN(logger) << "Max. database connection count may be non-optimal (use "
		                 << concurrency << " to match logical core count)" << LOG_SYNC;
	}

	ep::Pool<decltype(driver), ep::CheckinClean, ep::ExponentialGrowth>
		pool(driver, min_conns, max_conns, std::chrono::seconds(30));
	pool.logging_callback(std::bind(pool_log_callback, std::placeholders::_1,
	                                std::placeholders::_2, logger));

	LOG_INFO(logger) << "Initialising DAOs..." << LOG_SYNC; 
	auto user_dao = ember::dal::user_dao(pool);
	auto realm_dao = ember::dal::realm_dao(pool);
	auto ip_ban_dao = ember::dal::ip_ban_dao(pool); 
	auto ip_ban_cache = ember::IPBanCache(*ip_ban_dao);

	LOG_INFO(logger) << "Loading realm list..." << LOG_SYNC;
	ember::RealmList realm_list(realm_dao->get_realms());

	LOG_INFO(logger) << "Added " << realm_list.realms()->size() << " realm(s)"  << LOG_SYNC;

	for(auto& realm : *realm_list.realms() | boost::adaptors::map_values) {
		LOG_DEBUG(logger) << "#" << realm.id << " " << realm.name << LOG_SYNC;
	}

	// Start ASIO services
	LOG_INFO(logger) << "Starting thread pool with " << concurrency << " threads..." << LOG_SYNC;

	ember::ThreadPool thread_pool(concurrency);
	boost::asio::io_service service(concurrency);

	// Start metrics service
	auto metrics = std::make_unique<ember::Metrics>();

	if(args["metrics.enabled"].as<bool>()) {
		LOG_INFO(logger) << "Starting metrics service..." << LOG_SYNC;
		metrics = std::make_unique<ember::MetricsImpl>(
			service, args["metrics.statsd_host"].as<std::string>(),
			args["metrics.statsd_port"].as<std::uint16_t>()
		);
	}

	// Start login server
	const auto allowed_clients = client_versions();
	ember::Patcher patcher(allowed_clients, "temp");
	ember::LoginHandlerBuilder builder(logger, patcher, *user_dao, realm_list, *metrics);
	ember::LoginSessionBuilder s_builder(builder, thread_pool);

	auto interface = args["network.interface"].as<std::string>();
	auto port = args["network.port"].as<std::uint16_t>();
	auto tcp_no_delay = args["network.tcp_no_delay"].as<bool>();

	LOG_INFO(logger) << "Starting network service on " << interface << ":" << port << LOG_SYNC;

	ember::NetworkListener server(service, interface, port, tcp_no_delay, s_builder,
	                              ip_ban_cache, logger);

	// Start monitoring service
	std::unique_ptr<ember::Monitor> monitor;

	if(args["monitor.enabled"].as<bool>()) {
		LOG_INFO(logger) << "Starting monitoring service..." << LOG_SYNC;

		monitor = std::make_unique<ember::Monitor>(
			service, args["monitor.interface"].as<std::string>(),
			args["monitor.port"].as<std::uint16_t>(), *metrics
		);

		ember::install_net_monitor(*monitor, server, logger);
		ember::install_pool_monitor(*monitor, pool, logger);
	}

	service.dispatch([logger]() {
		LOG_INFO(logger) << "Login daemon started successfully" << LOG_SYNC;
	});
	
	// Spawn worker threads for ASIO
	std::vector<std::thread> workers;

	for(unsigned int i = 0; i < concurrency; ++i) {
		workers.emplace_back(static_cast<std::size_t(boost::asio::io_service::*)()>
			(&boost::asio::io_service::run), &service); 
	}

	service.run();

	LOG_INFO(logger) << "Login daemon shutting down..." << LOG_SYNC;

	for(auto& worker : workers) {
		worker.join();
	}
} catch(std::exception& e) {
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
}

/*
 * This vector defines the client builds that are allowed to connect to the
 * server. All builds in this list should be using the same protocol version.
 */
std::vector<ember::GameVersion> client_versions() {
	return {{1, 12, 1, 5875}, {1, 12, 2, 6005}};
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	//Command-line options
	po::options_description cmdline_opts("Generic options");
	cmdline_opts.add_options()
		("help", "Displays a list of available options")
		("database.config_path,d", po::value<std::string>(),
			"Path to the database configuration file")
		("config,c", po::value<std::string>()->default_value("login.conf"),
			"Path to the configuration file");

	po::positional_options_description pos; 
	pos.add("config", 1);

	//Config file options
	po::options_description config_opts("Login configuration options");
	config_opts.add_options()
		("network.interface,", po::value<std::string>()->required())
		("network.port", po::value<std::uint16_t>()->required())
		("network.tcp_no_delay", po::bool_switch()->default_value(true))
		("console_log.verbosity,", po::value<std::string>()->required())
		("console_log.filter-mask,", po::value<std::uint32_t>()->default_value(0))
		("remote_log.verbosity,", po::value<std::string>()->required())
		("remote_log.filter-mask,", po::value<std::uint32_t>()->default_value(0))
		("remote_log.service_name,", po::value<std::string>()->required())
		("remote_log.host,", po::value<std::string>()->required())
		("remote_log.port,", po::value<std::uint16_t>()->required())
		("file_log.verbosity,", po::value<std::string>()->required())
		("file_log.filter-mask,", po::value<std::uint32_t>()->default_value(0))
		("file_log.path,", po::value<std::string>()->default_value("login.log"))
		("file_log.timestamp_format,", po::value<std::string>())
		("file_log.mode,", po::value<std::string>()->required())
		("file_log.size_rotate,", po::value<std::uint32_t>()->required())
		("file_log.midnight_rotate,", po::bool_switch()->required())
		("file_log.log_timestamp,", po::bool_switch()->required())
		("file_log.log_severity,", po::bool_switch()->required())
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

/*
 * The concurrency level returned is usually the number of logical cores
 * in the machine but the standard doesn't guarantee that it won't be zero.
 * In that case, we just set the minimum concurrency level to two.
 */
unsigned int check_concurrency(el::Logger* logger) {
	unsigned int concurrency = std::thread::hardware_concurrency();

	if(!concurrency) {
		concurrency = 2;
		LOG_WARN(logger) << "Unable to determine concurrency level" << LOG_SYNC;
	}

#ifdef DEBUG_NO_THREADS
	return 0;
#else
	return concurrency;
#endif
}

void print_lib_versions(el::Logger* logger) {
	LOG_DEBUG(logger) << "Compiled with library versions: " << LOG_SYNC;
	LOG_DEBUG(logger) << "- Boost " << BOOST_VERSION / 100000 << "."
	                  << BOOST_VERSION / 100 % 1000 << "."
	                  << BOOST_VERSION % 100 << LOG_SYNC;
	LOG_DEBUG(logger) << "- " << Botan::version_string() << LOG_SYNC;
	LOG_DEBUG(logger) << "- " << ember::drivers::DriverType::name()
	                  << " (" << ember::drivers::DriverType::version() << ")" << LOG_SYNC;
}

void pool_log_callback(ep::Severity severity, const std::string& message, el::Logger* logger) {
	using ember::LF_DB_CONN_POOL;

	switch(severity) {
		case(ep::Severity::DEBUG):
			LOG_DEBUG_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
			break;
		case(ep::Severity::INFO):
			LOG_INFO_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
			break;
		case(ep::Severity::WARN):
			LOG_WARN_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
			break;
		case(ep::Severity::ERROR):
			LOG_ERROR_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
			break;
		case(ep::Severity::FATAL):
			LOG_FATAL_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
			break;
		default:
			LOG_ERROR_FILTER(logger, LF_DB_CONN_POOL) << "Unhandled pool log callback severity" << LOG_ASYNC;
			LOG_ERROR_FILTER(logger, LF_DB_CONN_POOL) << message << LOG_ASYNC;
	}	
}