/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountService.h"
#include "RealmService.h"
#include "FilterTypes.h"
#include "GameVersion.h"
#include "SessionBuilders.h"
#include "LoginHandlerBuilder.h"
#include "IntegrityData.h"
#include "MonitorCallbacks.h"
#include "NetworkListener.h"
#include "Patcher.h"
#include "RealmList.h"
#include <logger/Logging.h>
#include <conpool/ConnectionPool.h>
#include <conpool/Policies.h>
#include <conpool/drivers/AutoSelect.h>
#include <spark/Service.h>
#include <spark/ServiceDiscovery.h>
#include <shared/Banner.h>
#include <shared/util/LogConfig.h>
#include <shared/util/Utility.h>
#include <shared/metrics/MetricsImpl.h>
#include <shared/metrics/Monitor.h>
#include <shared/metrics/MetricsPoll.h>
#include <shared/threading/ThreadPool.h>
#include <shared/database/daos/IPBanDAO.h>
#include <shared/database/daos/PatchDAO.h>
#include <shared/database/daos/RealmDAO.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/IPBanCache.h>
#include <shared/util/xoroshiro128plus.h>
#include <botan/version.h>
#include <boost/asio/io_context.hpp>
#include <boost/version.hpp>
#include <boost/program_options.hpp>
#include <boost/range/adaptor/map.hpp>
#include <pcre.h>
#include <zlib.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <span>
#include <string_view>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace ep = ember::connection_pool;
namespace po = boost::program_options;
namespace ba = boost::asio;
using namespace std::chrono_literals;
using namespace ember;

void print_lib_versions(log::Logger* logger);
std::vector<ember::GameVersion> client_versions();
unsigned int check_concurrency(log::Logger* logger);
int launch(const po::variables_map& args, log::Logger* logger);
po::variables_map parse_arguments(int argc, const char* argv[]);
void pool_log_callback(ep::Severity, std::string_view message, log::Logger* logger);

const char* APP_NAME = "Login Daemon";

/*
 * We want to do the minimum amount of work required to get 
 * logging facilities and crash handlers up and running in main.
 *
 * Exceptions that aren't derived from std::exception are
 * left to the crash handler since we can't get useful information
 * from them.
 */
int main(int argc, const char* argv[]) try {
	print_banner(APP_NAME);
	util::set_window_title(APP_NAME);

	const po::variables_map args = parse_arguments(argc, argv);

	auto logger = util::init_logging(args);
	log::set_global_logger(logger.get());
	LOG_INFO(logger) << "Logger configured successfully" << LOG_SYNC;

	print_lib_versions(logger.get());
	const auto ret = launch(args, logger.get());
	LOG_INFO(logger) << APP_NAME << " terminated" << LOG_SYNC;
	return ret;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

int launch(const po::variables_map& args, log::Logger* logger) try {
#ifdef DEBUG_NO_THREADS
	LOG_WARN(logger) << "Compiled with DEBUG_NO_THREADS!" << LOG_SYNC;
#endif

	LOG_INFO(logger) << "Seeding xorshift RNG..." << LOG_SYNC;
	Botan::AutoSeeded_RNG rng;
	auto seed_bytes = std::as_writable_bytes(std::span(rng::xorshift::seed));
	rng.randomize(reinterpret_cast<std::uint8_t*>(seed_bytes.data()), seed_bytes.size_bytes());

	LOG_INFO(logger) << "Loading patch data..." << LOG_SYNC;

	unsigned int concurrency = check_concurrency(logger);

	LOG_INFO(logger) << "Initialising database driver..."<< LOG_SYNC;
	const auto& db_config_path = args["database.config_path"].as<std::string>();
	auto driver(drivers::init_db_driver(db_config_path));
	auto min_conns = args["database.min_connections"].as<unsigned short>();
	auto max_conns = args["database.max_connections"].as<unsigned short>();

	LOG_INFO(logger) << "Initialising database connection pool..."<< LOG_SYNC;

	if(max_conns != concurrency) {
		LOG_WARN(logger) << "Max. database connection count may be non-optimal (use "
		                 << concurrency << " to match logical core count)" << LOG_SYNC;
	}

	ep::Pool<decltype(driver), ep::CheckinClean, ep::ExponentialGrowth> pool(driver, min_conns, max_conns, 30s);

	pool.logging_callback([logger](auto severity, auto message) {
		pool_log_callback(severity, message, logger);
	});

	LOG_INFO(logger) << "Initialising DAOs..." << LOG_SYNC; 
	auto user_dao = dal::user_dao(pool);
	auto realm_dao = dal::realm_dao(pool);
	auto patch_dao = dal::patch_dao(pool);
	auto ip_ban_dao = dal::ip_ban_dao(pool); 
	auto ip_ban_cache = IPBanCache(ip_ban_dao->all_bans());

	// Load integrity, patch and survey data
	LOG_INFO(logger) << "Loading client integrity validation data..." << LOG_SYNC;
	std::unique_ptr<IntegrityData> exe_data;

	const auto allowed_clients = client_versions(); // move

	if(args["integrity.enabled"].as<bool>()) {
		const auto& bin_path = args["integrity.bin_path"].as<std::string>();
		exe_data = std::make_unique<IntegrityData>(allowed_clients, bin_path);
	}

	LOG_INFO(logger) << "Loading patch data..." << LOG_SYNC;

	auto patches = Patcher::load_patches(
		args["patches.bin_path"].as<std::string>(), *patch_dao, logger
	);

	Patcher patcher(allowed_clients, patches);

	if(args["survey.enabled"].as<bool>()) {
		LOG_INFO(logger) << "Loading survey data..." << LOG_SYNC;
		patcher.set_survey(
			args["survey.bin_path"].as<std::string>(),
			args["survey.id"].as<std::uint32_t>()
		);
	}

	LOG_INFO(logger) << "Loading realm list..." << LOG_SYNC;
	RealmList realm_list(realm_dao->get_realms());

	LOG_INFO(logger) << "Added " << realm_list.realms()->size() << " realm(s)"  << LOG_SYNC;

	for(auto& realm : *realm_list.realms() | boost::adaptors::map_values) {
		LOG_DEBUG(logger) << "#" << realm.id << " " << realm.name << LOG_SYNC;
	}

	// Start ASIO service
	LOG_INFO(logger) << "Starting thread pool with " << concurrency << " threads..." << LOG_SYNC;

	ThreadPool thread_pool(concurrency);
	boost::asio::io_context service(concurrency);

	// Start Spark services
	LOG_INFO(logger) << "Starting Spark service..." << LOG_SYNC;
	const auto& s_address = args["spark.address"].as<std::string>();
	auto s_port = args["spark.port"].as<std::uint16_t>();
	const auto& mcast_group = args["spark.multicast_group"].as<std::string>();
	const auto& mcast_iface = args["spark.multicast_interface"].as<std::string>();
	auto mcast_port = args["spark.multicast_port"].as<std::uint16_t>();
	auto spark_filter = log::Filter(FilterType::LF_SPARK);

	spark::Service spark("login", service, s_address, s_port, logger);
	spark::ServiceDiscovery discovery(service, s_address, s_port, mcast_iface, mcast_group,
	                               mcast_port, logger);

	AccountService acct_svc(spark, discovery, logger);
	RealmService realm_svc(realm_list, spark, discovery, logger);

	// Start metrics service
	auto metrics = std::make_unique<Metrics>();

	if(args["metrics.enabled"].as<bool>()) {
		LOG_INFO(logger) << "Starting metrics service..." << LOG_SYNC;
		metrics = std::make_unique<MetricsImpl>(
			service, args["metrics.statsd_host"].as<std::string>(),
			args["metrics.statsd_port"].as<std::uint16_t>()
		);
	}

	// Start login server
	LoginHandlerBuilder builder(logger, patcher, exe_data.get(), *user_dao, acct_svc,
	                            realm_list, *metrics, args["locale.enforce"].as<bool>());
	LoginSessionBuilder s_builder(builder, thread_pool);

	const auto& interface = args["network.interface"].as<std::string>();
	auto port = args["network.port"].as<std::uint16_t>();
	auto tcp_no_delay = args["network.tcp_no_delay"].as<bool>();

	LOG_INFO(logger) << "Starting network service on " << interface << ":" << port << LOG_SYNC;

	NetworkListener server(service, interface, port, tcp_no_delay, s_builder, ip_ban_cache,
	                              logger, *metrics);

	// Start monitoring service
	std::unique_ptr<Monitor> monitor;

	if(args["monitor.enabled"].as<bool>()) {
		LOG_INFO(logger) << "Starting monitoring service..." << LOG_SYNC;

		monitor = std::make_unique<Monitor>(	
			service, args["monitor.interface"].as<std::string>(),
			args["monitor.port"].as<std::uint16_t>()
		);

		install_net_monitor(*monitor, server, logger);
		install_pool_monitor(*monitor, pool, logger);
	}

	// Start metrics polling
	MetricsPoll poller(service, *metrics);

	poller.add_source([&pool](Metrics& metrics) {
		metrics.gauge("db_connections", pool.size());
	}, 5s);

	poller.add_source([&server](Metrics& metrics) {
		metrics.gauge("sessions", server.connection_count());
	}, 5s);

	// Misc. information
	LOG_INFO_FMT(logger, "Max allowed sockets: {}", util::max_sockets_desc());
	std::string builds;

	for(const auto& client : allowed_clients) {
		builds += std::to_string(client.build) + " ";
	}

	LOG_INFO_FMT(logger, "Allowed client builds: {}", builds);

	// All done setting up
	service.dispatch([logger]() {
		LOG_INFO(logger) << APP_NAME << " started successfully" << LOG_SYNC;
	});
	
	// Spawn worker threads for ASIO
	std::vector<std::thread> workers;

	// start from one to take the main thread into account
	for(unsigned int i = 1; i < concurrency; ++i) {
		workers.emplace_back(static_cast<std::size_t(boost::asio::io_context::*)()>
			(&boost::asio::io_context::run), &service); 
	}

	service.run();

	LOG_INFO(logger) << APP_NAME << " shutting down..." << LOG_SYNC;

	for(auto& worker : workers) {
		worker.join();
	}

	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
	return EXIT_FAILURE;
}

/*
 * This vector defines the client builds that are allowed to connect to the
 * server. All builds in this list should be using the same protocol version.
 */
std::vector<GameVersion> client_versions() {
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
		("locale.enforce", po::value<bool>()->required())
		("patches.bin_path", po::value<std::string>()->required())
		("survey.enabled", po::value<bool>()->default_value(false))
		("survey.bin_path", po::value<std::string>()->required())
		("survey.id", po::value<std::uint32_t>()->required())
		("integrity.enabled", po::value<bool>()->default_value(false))
		("integrity.bin_path", po::value<std::string>()->required())
		("spark.address", po::value<std::string>()->required())
		("spark.port", po::value<std::uint16_t>()->required())
		("spark.multicast_interface", po::value<std::string>()->required())
		("spark.multicast_group", po::value<std::string>()->required())
		("spark.multicast_port", po::value<std::uint16_t>()->required())
		("network.interface", po::value<std::string>()->required())
		("network.port", po::value<std::uint16_t>()->required())
		("network.tcp_no_delay", po::value<bool>()->default_value(true))
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
		("file_log.path", po::value<std::string>()->default_value("login.log"))
		("file_log.timestamp_format", po::value<std::string>())
		("file_log.mode", po::value<std::string>()->required())
		("file_log.size_rotate", po::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", po::bool_switch()->required())
		("file_log.log_timestamp", po::value<bool>()->required())
		("file_log.log_severity", po::value<bool>()->required())
		("database.config_path", po::value<std::string>()->required())
		("database.min_connections", po::value<unsigned short>()->required())
		("database.max_connections", po::value<unsigned short>()->required())
		("metrics.enabled", po::value<bool>()->required())
		("metrics.statsd_host", po::value<std::string>()->required())
		("metrics.statsd_port", po::value<std::uint16_t>()->required())
		("monitor.enabled", po::value<bool>()->required())
		("monitor.interface", po::value<std::string>()->required())
		("monitor.port", po::value<std::uint16_t>()->required());

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

/*
 * The concurrency level returned is usually the number of logical cores
 * in the machine but the standard doesn't guarantee that it won't be zero.
 * In that case, we just set the minimum concurrency level to two.
 */
unsigned int check_concurrency(log::Logger* logger) {
#ifdef DEBUG_NO_THREADS
	return 0;
#endif

	unsigned int concurrency = std::thread::hardware_concurrency();

	if(!concurrency) {
		concurrency = 2;
		LOG_WARN(logger) << "Unable to determine concurrency level" << LOG_SYNC;
	}

	return concurrency;
}

void print_lib_versions(log::Logger* logger) {
	LOG_DEBUG(logger) << "Compiled with library versions: " << LOG_SYNC;
	LOG_DEBUG(logger) << "- Boost " << BOOST_VERSION / 100000 << "."
	                  << BOOST_VERSION / 100 % 1000 << "."
	                  << BOOST_VERSION % 100 << LOG_SYNC;
	LOG_DEBUG(logger) << "- " << Botan::version_string() << LOG_SYNC;
	LOG_DEBUG(logger) << "- " << drivers::DriverType::name()
	                  << " (" << drivers::DriverType::version() << ")" << LOG_SYNC;
	LOG_DEBUG(logger) << "- PCRE " << PCRE_MAJOR << "." << PCRE_MINOR << LOG_SYNC;
	LOG_DEBUG(logger) << "- Zlib " << ZLIB_VERSION << LOG_SYNC;
}

void pool_log_callback(ep::Severity severity, std::string_view message, log::Logger* logger) {
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