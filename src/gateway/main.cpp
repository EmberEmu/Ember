/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Config.h"
#include "Locator.h"
#include "FilterTypes.h"
#include "RealmQueue.h"
#include "AccountService.h"
#include "EventDispatcher.h"
#include "CharacterService.h"
#include "RealmService.h"
#include "NetworkListener.h"
#include <spark/Spark.h>
#include <conpool/ConnectionPool.h>
#include <conpool/Policies.h>
#include <conpool/drivers/AutoSelect.h>
#include <logger/Logging.h>
#include <shared/Banner.h>
#include <shared/util/EnumHelper.h>
#include <shared/Version.h>
#include <shared/util/Utility.h>
#include <shared/util/LogConfig.h>
#include <stun/Client.h>
#include <stun/Utility.h>
#include <dbcreader/DBCReader.h>
#include <shared/database/daos/RealmDAO.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/threading/ServicePool.h>
#include <shared/util/xoroshiro128plus.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <botan/auto_rng.h>
#include <chrono>
#include <iostream>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <stdexcept>

const char* APP_NAME = "Realm Gateway";

namespace ep = ember::connection_pool;
namespace po = boost::program_options;
namespace ba = boost::asio;

using namespace std::chrono_literals;
using namespace std::placeholders;

namespace ember {

using StunPair = std::pair<std::unique_ptr<stun::Client>, std::future<stun::MappedResult>>;

int launch(const po::variables_map& args, log::Logger* logger);
unsigned int check_concurrency(log::Logger* logger); // todo, move
po::variables_map parse_arguments(int argc, const char* argv[]);
void pool_log_callback(ep::Severity, std::string_view message, log::Logger* logger);
const std::string& category_name(const Realm& realm, const dbc::DBCMap<dbc::Cfg_Categories>& dbc);
StunPair start_stun_query(const po::variables_map& args, log::Logger* logger);
void stun_log_callback(const stun::Verbosity verbosity, const stun::Error reason,
                       log::Logger* logger);
void handle_stun_results(stun::Client* client, Realm* realm,
                         std::future<stun::MappedResult> result,
                         std::uint16_t port,
                         log::Logger* logger);

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
	using namespace ember;

	print_banner(APP_NAME);
	util::set_window_title(APP_NAME);

	const po::variables_map args = parse_arguments(argc, argv);

	auto logger = util::init_logging(args);
	log::set_global_logger(logger.get());
	LOG_INFO(logger) << "Logger configured successfully" << LOG_SYNC;

	const auto ret = launch(args, logger.get());
	LOG_INFO(logger) << APP_NAME << " terminated" << LOG_SYNC;
	return ret;
} catch(const std::exception& e) {
	std::cerr << e.what();
	return EXIT_FAILURE;
}

namespace ember {

int launch(const po::variables_map& args, log::Logger* logger) try {
#ifdef DEBUG_NO_THREADS
	LOG_WARN(logger) << "Compiled with DEBUG_NO_THREADS!" << LOG_SYNC;
#endif

	auto [stun, stun_res ] = start_stun_query(args, logger);

	LOG_INFO(logger) << "Seeding xorshift RNG..." << LOG_SYNC;
	Botan::AutoSeeded_RNG rng;
	auto seed_bytes = std::as_writable_bytes(std::span(ember::rng::xorshift::seed));
	rng.randomize(reinterpret_cast<std::uint8_t*>(seed_bytes.data()), seed_bytes.size_bytes());

	LOG_INFO(logger) << "Loading DBC data..." << LOG_SYNC;
	dbc::DiskLoader loader(args["dbc.path"].as<std::string>(), [&](auto message) {
		LOG_DEBUG(logger) << message << LOG_SYNC;
	});

	auto dbc_store = loader.load({"AddonData", "Cfg_Categories"});

	LOG_INFO(logger) << "Resolving DBC references..." << LOG_SYNC;
	dbc::link(dbc_store);

	LOG_INFO(logger) << "Initialising database driver..." << LOG_SYNC;
	const auto& db_config_path = args["database.config_path"].as<std::string>();
	auto driver(ember::drivers::init_db_driver(db_config_path));

	LOG_INFO(logger) << "Initialising database connection pool..." << LOG_SYNC;
	ep::Pool<decltype(driver), ep::CheckinClean, ep::ExponentialGrowth> pool(driver, 1, 1, 30s);
	
	pool.logging_callback([logger](auto severity, auto message) {
		pool_log_callback(severity, message, logger);
	});

	LOG_INFO(logger) << "Initialising DAOs..." << LOG_SYNC;
	auto realm_dao = ember::dal::realm_dao(pool);

	LOG_INFO(logger) << "Retrieving realm information..."<< LOG_SYNC;
	auto realm = realm_dao->get_realm(args["realm.id"].as<unsigned int>());
	
	if(!realm) {
		throw std::invalid_argument("Invalid realm ID supplied in configuration.");
	}
	
	// Validate category & region
	const auto& cat_name = category_name(*realm, dbc_store.cfg_categories);

	LOG_INFO(logger) << "Serving as gateway for " << realm->name
	                 << " (" << cat_name << ")" << LOG_SYNC;

	util::set_window_title(std::string(APP_NAME) + " - " + realm->name);

	// Set config
	Config config;
	config.max_slots = args["realm.max_slots"].as<unsigned int>();
	config.list_zone_hide = args["quirks.list_zone_hide"].as<bool>();
	config.realm = &realm.value();

	// Determine concurrency level
	unsigned int concurrency = check_concurrency(logger);

	if(args.count("misc.concurrency")) {
		concurrency = args["misc.concurrency"].as<unsigned int>();
	}

	// Start ASIO service pool
	LOG_INFO(logger) << "Starting service pool with " << concurrency << " threads..." << LOG_SYNC;
	ServicePool service_pool(concurrency);

	LOG_INFO(logger) << "Starting event dispatcher..." << LOG_SYNC;
	EventDispatcher dispatcher(service_pool);

	LOG_INFO(logger) << "Starting Spark service..." << LOG_SYNC;
	const auto& s_address = args["spark.address"].as<std::string>();
	auto s_port = args["spark.port"].as<std::uint16_t>();
	const auto& mcast_group = args["spark.multicast_group"].as<std::string>();
	const auto& mcast_iface = args["spark.multicast_interface"].as<std::string>();
	auto mcast_port = args["spark.multicast_port"].as<std::uint16_t>();
	auto spark_filter = log::Filter(FilterType::LF_SPARK);

	auto& service = service_pool.get_service();

	spark::Service spark("gateway-" + realm->name, service, s_address, s_port, logger);
	spark::ServiceDiscovery discovery(service, s_address, s_port, mcast_iface, mcast_group,
	                                  mcast_port, logger);

	const auto port = args["network.port"].as<std::uint16_t>();

	if(stun) {
		handle_stun_results(&*stun, &*realm, std::move(stun_res), port, logger);
		stun.reset();
	}

	const auto& msg = std::format("Realm will be advertised on {}", realm->ip);
	LOG_INFO(logger) << msg << LOG_SYNC;

	RealmQueue queue_service(service_pool.get_service());
	RealmService realm_svc(*realm, spark, discovery, logger);
	AccountService acct_svc(spark, discovery, logger);
	CharacterService char_svc(spark, discovery, config, logger);
	
	// set services - not the best design pattern but it'll do for now
	Locator::set(&dispatcher);
	Locator::set(&queue_service);
	Locator::set(&realm_svc);
	Locator::set(&acct_svc);
	Locator::set(&char_svc);
	Locator::set(&config);
	
	// Misc. information
	const auto max_socks = util::max_sockets_desc();
	LOG_INFO_FMT(logger, "Max allowed sockets: {}", max_socks);

	// Start network listener
	const auto& interface = args["network.interface"].as<std::string>();
	const auto tcp_no_delay = args["network.tcp_no_delay"].as<bool>();

	LOG_INFO(logger) << "Starting network service on " << interface << ":" << port << LOG_SYNC;

	NetworkListener server(service_pool, interface, port, tcp_no_delay, logger);

	boost::asio::io_context wait_svc;
	boost::asio::signal_set signals(wait_svc, SIGINT, SIGTERM);

	signals.async_wait([&](const boost::system::error_code& error, int signal) {
		LOG_DEBUG(logger) << "Received signal " << signal << LOG_SYNC;
	});

	service.dispatch([&, logger]() {
		realm_svc.set_online();
		LOG_INFO(logger) << APP_NAME << " started successfully" << LOG_SYNC;
	});

	service_pool.run();
	wait_svc.run();

	LOG_INFO(logger) << APP_NAME << " shutting down..." << LOG_SYNC;
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
	return EXIT_FAILURE;
}

const std::string& category_name(const Realm& realm, const dbc::DBCMap<dbc::Cfg_Categories>& dbc) {
	for(auto&& [k, record] : dbc) {
		if(record.category == realm.category && record.region == realm.region) {
			return record.name.en_gb;
		}
	}

	throw std::invalid_argument("Unknown category/region combination in database");
}

void handle_stun_results(stun::Client* client, Realm* realm,
                         std::future<stun::MappedResult> future,
                         const std::uint16_t port,
						 log::Logger* logger) {
	const auto result = future.get();

	if(!result) {
		const auto& msg = std::format(
			"STUN: Query failed ({}), falling back to realm config {}",
			stun::to_string(result.error().reason), realm->ip
		);

		LOG_ERROR(logger) << msg << LOG_SYNC;
		return;
	}

	const auto& ip = stun::extract_ip_to_string(*result);
	realm->ip = std::format("{}:{}", ip, port);

	LOG_INFO(logger)
		<< std::format("STUN: Binding request succeeded ({})", ip)
		<< LOG_SYNC;

	const auto nat = client->nat_present().get();

	if(!nat) {
		const auto& msg = std::format(
			"STUN: Unable to determine if gateway is behind NAT ({})",
			stun::to_string(nat.error().reason)
		);

		LOG_WARN(logger) << msg << LOG_SYNC;
		return;
	}

	if(*nat) {
		const auto& msg = std::format(
			"STUN: Gateway appears to be behind NAT, "
			"forward port {} for external access", port
		);

		LOG_INFO(logger) << msg << LOG_SYNC;
	} else {
		LOG_INFO(logger)
			<< "STUN: Gateway does not appear to be behind NAT - "
				"server is available online (firewall rules permitting)"
			<< LOG_SYNC;
	}
}

StunPair start_stun_query(const po::variables_map& args, log::Logger* logger) {
	if(!args["stun.enabled"].as<bool>()) {
		return {};
	}

	LOG_INFO(logger) << "Starting STUN query..." << LOG_SYNC;
	const auto& proto_arg = args["stun.protocol"].as<std::string>();

	if(proto_arg != "tcp" && proto_arg != "udp") {
		throw std::invalid_argument("Invalid STUN protocol argument");
	}

	auto stun = std::make_unique<stun::Client>(
		args["network.interface"].as<std::string>(),
		args["stun.server"].as<std::string>(),
		args["stun.port"].as<std::uint16_t>(),
		proto_arg == "tcp"? stun::Protocol::TCP : stun::Protocol::UDP
	);

	stun->log_callback([logger](const stun::Verbosity verbosity, const stun::Error reason) {
		stun_log_callback(verbosity, reason, logger);
	});

	auto stun_res = stun->external_address();

	return { std::move(stun), std::move(stun_res) };
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
		("quirks.list_zone_hide", po::value<bool>()->required())
		("dbc.path", po::value<std::string>()->required())
		("misc.concurrency", po::value<unsigned int>())
		("realm.id", po::value<unsigned int>()->required())
		("realm.max_slots", po::value<unsigned int>()->required())
		("realm.reserved_slots", po::value<unsigned int>()->required())
		("spark.address", po::value<std::string>()->required())
		("spark.port", po::value<std::uint16_t>()->required())
		("spark.multicast_interface", po::value<std::string>()->required())
		("spark.multicast_group", po::value<std::string>()->required())
		("spark.multicast_port", po::value<std::uint16_t>()->required())
		("stun.enabled", po::value<bool>()->required())
		("stun.server", po::value<std::string>()->required())
		("stun.port", po::value<std::uint16_t>()->required())
		("stun.protocol", po::value<std::string>()->required())
		("network.interface", po::value<std::string>()->required())
		("network.port", po::value<std::uint16_t>()->required())
		("network.tcp_no_delay", po::value<bool>()->required())
		("network.compression", po::value<std::uint8_t>()->required())
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
		("file_log.path", po::value<std::string>()->default_value("gateway.log"))
		("file_log.timestamp_format", po::value<std::string>())
		("file_log.mode", po::value<std::string>()->required())
		("file_log.size_rotate", po::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", po::value<bool>()->required())
		("file_log.log_timestamp", po::value<bool>()->required())
		("file_log.log_severity", po::value<bool>()->required())
		("database.config_path", po::value<std::string>()->required())
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
		const std::string message("Unable to open configuration file: " + config_path);
		throw std::invalid_argument(message);
	}

	po::store(po::parse_config_file(ifs, config_opts), options);
	po::notify(options);

	return options;
}

void pool_log_callback(ep::Severity severity, std::string_view message, log::Logger* logger) {
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

void stun_log_callback(const stun::Verbosity verbosity, const stun::Error reason,
                       log::Logger* logger) {
	switch(verbosity) {
		case stun::Verbosity::STUN_LOG_TRIVIAL:
			LOG_TRACE(logger) << "[stun] " << reason << LOG_SYNC;
			break;
		case stun::Verbosity::STUN_LOG_DEBUG:
			LOG_DEBUG(logger) << "[stun] " << reason << LOG_SYNC;
			break;
		case stun::Verbosity::STUN_LOG_INFO:
			LOG_INFO(logger) << "[stun] " << reason << LOG_SYNC;
			break;
		case stun::Verbosity::STUN_LOG_WARN:
			LOG_WARN(logger) << "[stun] " << reason << LOG_SYNC;
			break;
		case stun::Verbosity::STUN_LOG_ERROR:
			LOG_ERROR(logger) << "[stun] " << reason << LOG_SYNC;
			break;
		case stun::Verbosity::STUN_LOG_FATAL:
			LOG_FATAL(logger) << "[stun] " << reason << LOG_SYNC;
			break;
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

#ifdef DEBUG_NO_THREADS
	return 0;
#else
	return concurrency;
#endif
}

} // ember