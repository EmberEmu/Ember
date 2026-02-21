/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include "Config.h"
#include "Locator.h"
#include "FilterTypes.h"
#include "RealmQueue.h"
#include "AccountClient.h"
#include "EventDispatcher.h"
#include "CharacterClient.h"
#include "RealmService.h"
#include "NetworkListener.h"
#include <conpool/ConnectionPool.h>
#include <conpool/Policies.h>
#include <conpool/drivers/AutoSelect.h>
#include <dbcreader/Reader.h>
#include <logger/Logger.h>
#include <nsd/NSD.h>
#include <ports/Forward.h>
#include <ports/Utility.h>
#include <spark/Server.h>
#include <shared/utility/EnumHelper.h>
#include <shared/database/daos/RealmDAO.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/game/GameVersion.h>
#include <shared/game/Utility.h>
#include <shared/threading/ServicePool.h>
#include <shared/threading/Utility.h>
#include <shared/utility/cstring_view.hpp>
#include <shared/utility/Utility.h>
#include <shared/utility/LogConfig.h>
#include <shared/utility/STUN.h>
#include <shared/utility/xoroshiro128plus.h>
#include <stun/Client.h>
#include <stun/Utility.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/version.hpp>
#include <botan/auto_rng.h>
#include <botan/version.h>
#include <pcre.h>
#include <zlib.h>
#include <chrono>
#include <format>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <stdexcept>
#include <cstdlib>

using namespace std::chrono_literals;

namespace po = boost::program_options;
namespace ep = ember::connection_pool;

namespace ember::gateway {

std::optional<Realm> load_realm(const po::variables_map& args, log::Logger& logger);
void pool_log_callback(ep::Severity, std::string_view message, log::Logger& logger);
std::string_view category_name(const Realm& realm, const dbc::Store<dbc::Cfg_Categories>& dbc);
void print_lib_versions(log::Logger& logger);

/*
 * Starts Asio worker threads, blocking until the launch thread exits
 * upon error or signal handling.
 * 
 * io_context is only stopped after the thread joins to ensure that all
 * services can cleanly shut down upon destruction without requiring
 * explicit shutdown() calls in a signal handler.
 */
int Service::run(const po::variables_map& args) try {
	const auto concurrency = thread::hardware_concurrency(logger);

	// Start Asio service pool
	LOG_INFO_SYNC(logger, "Starting service pool with {} threads", concurrency);
	ServicePool service_pool(concurrency, BOOST_ASIO_CONCURRENCY_HINT_UNSAFE_IO);
	service_pool.run();

	std::thread thread([&]() {
		thread::set_name("Launcher");
		launch(args, service_pool);
	});

	thread.join();

	if(eptr) {
		std::rethrow_exception(eptr);
	}

	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
	return EXIT_FAILURE;
}

void Service::stop() {
	stop_flag.release();
}

void Service::launch(const po::variables_map& args, ServicePool& service_pool) try {
#ifdef DEBUG_NO_THREADS
	LOG_WARN_SYNC(logger, "Compiled with DEBUG_NO_THREADS!");
#endif
	print_lib_versions(logger);

	const auto allowed_builds = args["realm.builds"].as<std::vector<GameVersion>>();
	std::string builds;

	for(const auto& client : allowed_builds) {
		builds += to_string(client) + " ";
	}

	LOG_INFO_SYNC(logger, "Allowed client builds: {}", builds);

	auto stun = create_stun_client(args);
	const auto stun_enabled = args["stun.enabled"].as<bool>();
	const auto forward_enabled = args["forward.enabled"].as<bool>();

	if(forward_enabled && !stun_enabled) {
		throw std::invalid_argument("Port forwarding requires STUN to be enabled");
	}

	std::future<stun::MappedResult> stun_res;

	if(stun_enabled) {
		stun.log_callback([&](const stun::Verbosity verbosity, const stun::Error reason) {
			stun_log_callback(verbosity, reason, logger);
		});

		LOG_INFO_SYNC(logger, "Starting STUN query...");
		stun_res = stun.external_address();
	}

	LOG_INFO_SYNC(logger, "Seeding xorshift RNG...");
	Botan::AutoSeeded_RNG rng;
	auto seed_bytes = std::as_writable_bytes(std::span(rng::xorshift::seed));
	rng.randomize(reinterpret_cast<std::uint8_t*>(seed_bytes.data()), seed_bytes.size_bytes());

	LOG_INFO_SYNC(logger, "Loading DBC data...");
	dbc::DiskLoader loader(args["dbc.path"].as<std::string>(), [&](auto message) {
		LOG_DEBUG(logger) << message << LOG_SYNC;
	});

	auto dbc_store = loader.load("AddonData", "Cfg_Categories");

	LOG_INFO_SYNC(logger, "Resolving DBC references...");
	dbc::link(dbc_store);

	const auto realm_id = args["realm.id"].as<unsigned int>();
	auto realm = load_realm(args, logger);
	
	if(!realm) {
		throw std::invalid_argument(
			std::format("(Configured realm ID {} does not exist in database.", realm_id)
		);
	}
	
	const auto& title = std::format("{} - {}", APP_NAME, realm->name);
	utility::set_window_title(title);

	// Validate category & region
	const auto& cat_name = category_name(*realm, dbc_store.cfg_categories);
	LOG_INFO_SYNC(logger, "Serving as gateway for {} ({})", realm->name, cat_name);

	// Determine concurrency level
	unsigned int concurrency = thread::hardware_concurrency(logger);

	if(args.count("misc.concurrency")) {
		concurrency = args["misc.concurrency"].as<unsigned int>();
	}

	LOG_INFO_SYNC(logger, "Starting event dispatcher...");
	EventDispatcher dispatcher(service_pool, logger);

	LOG_INFO_SYNC(logger, "Starting Spark service...");
	const auto& s_address = args["spark.address"].as<std::string>();
	auto s_port = args["spark.port"].as<std::uint16_t>();

	auto& service = service_pool.get();
	const auto port = args["network.port"].as<std::uint16_t>();
	const auto& interface = args["network.interface"].as<std::string>();
	const auto tcp_no_delay = args["network.tcp_no_delay"].as<bool>();

	auto update_realm_address = [](Realm& realm) {
		realm.address = std::format("{}:{}", realm.ip, realm.port);
	};

	// If the database port differs from the config file port, use the config file port
	if(port != realm->port) {
		LOG_WARN_SYNC(
			logger, "Configured port {} differs from database entry port {}, using {}",
			port, realm->port, port
		);

		realm->port = port;
		update_realm_address(*realm);
	}

	// Retrieve STUN result and start port forwarding if enabled and STUN succeeded
	std::unique_ptr<ports::Forward> forward;

	if(stun_enabled) {
		LOG_INFO_SYNC(logger, "Waiting on STUN result...");

		const auto result = stun_res.get();
		log_stun_result(stun, result, port, logger);

		if(result) {
			realm->ip = stun::extract_ip_to_string(*result);
			update_realm_address(*realm);
		}

		if(result && forward_enabled) {
			const auto nat = stun.nat_present().get();

			if(nat && !*nat) {
				LOG_WARN_SYNC(logger, "Port forwarding skipped as we do not appear to be behind NAT");
			} else {
				const auto& mode = args["forward.method"].as<ports::Forward::Method>();
				const auto& gateway = args["forward.gateway"].as<std::string>();

				forward = std::make_unique<ports::Forward>(
					logger, service, mode, interface, gateway, port
				);
			}
		} else if(!result && forward_enabled) {
			LOG_WARN_SYNC(logger, "Port forwarding skipped due to STUN failure");
		}
	}

	Config config {
		.realm = *realm,
		.realm_id = realm_id,
		.max_slots = args["realm.max_slots"].as<unsigned int>(),
		.auth_timeout = std::chrono::seconds(args["realm.auth_timeout"].as<unsigned int>()),
		.char_list_timeout = std::chrono::seconds(args["realm.char_list_timeout"].as<unsigned int>())
	};

	LOG_INFO_SYNC(logger, "Realm will be advertised on {}", realm->address);

	RealmQueue realm_queue(service_pool.get());
	
	LOG_INFO_SYNC(logger, "Starting RPC services...");
	spark::Server spark(service_pool.get(), "realm", s_address, s_port, logger);
	RealmService realm_svc(spark, *realm, logger);
	AccountClient acct_svc(spark, logger);
	CharacterClient char_svc(spark, config, logger);

	const auto& nsd_host = args["nsd.host"].as<std::string>();
	const auto nsd_port = args["nsd.port"].as<std::uint16_t>();

	NetworkServiceDiscovery nds(spark, nsd_host, nsd_port, logger);

	// set services - not the best design pattern but it'll do for now
	Locator::set(allowed_builds);
	Locator::set(&dispatcher);
	Locator::set(&realm_queue);
	Locator::set(&realm_svc);
	Locator::set(&acct_svc);
	Locator::set(&char_svc);
	Locator::set(&config);
	
	// Misc. information
	const auto max_socks = utility::max_sockets_desc();
	LOG_INFO_SYNC(logger, "Max allowed sockets: {}", max_socks);

	// Start network listener
	LOG_INFO_SYNC(logger, "Starting network service...");

	NetworkListener server(service_pool, interface, port, tcp_no_delay, logger);

	LOG_INFO_SYNC(logger, "Started network service on {}:{}", interface, server.port());

	boost::asio::dispatch(service, [&]() {
		realm_svc.set_online();
		LOG_INFO_SYNC(logger, "{} started successfully", APP_NAME);
	});

	stop_flag.acquire();
	LOG_INFO_SYNC(logger, "{} shutting down...", APP_NAME);
} catch(...) {
	eptr = std::current_exception();
}

std::string_view category_name(const Realm& realm, const dbc::Store<dbc::Cfg_Categories>& dbc) {
	for(auto& record : dbc | std::views::values) {
		if(record.category == realm.category && record.region == realm.region) {
			return record.name.en_gb;
		}
	}

	throw std::invalid_argument("Unknown category/region combination in database");
}

/*
 * Split from launch() as the DB connection is only needed for
 * loading the initial realm information. If the gateway requires
 * connections elsewhere in the future, this should be merged back.
 */
std::optional<Realm> load_realm(const po::variables_map& args, log::Logger& logger) {
	LOG_INFO_SYNC(logger, "Initialising database driver...");
	const auto& db_config_path = args["database.config_path"].as<std::string>();
	auto driver(drivers::init_db_driver(db_config_path, "login"));

	LOG_INFO_SYNC(logger, "Initialising database connection pool...");
	ep::Pool<decltype(driver), ep::CheckinClean, ep::ExponentialGrowth> pool(driver, 1, 1, 30s);
	
	pool.logging_callback([&](auto severity, auto message) {
		pool_log_callback(severity, message, logger);
	});

	LOG_INFO_SYNC(logger, "Initialising DAOs...");
	auto realm_dao = dal::realm_dao(pool);

	LOG_INFO_SYNC(logger, "Retrieving realm information...");
	return realm_dao.get_realm(args["realm.id"].as<unsigned int>());
}

void pool_log_callback(ep::Severity severity, std::string_view message, log::Logger& logger) {
	switch(severity) {
		case ep::Severity::debug:
			LOG_DEBUG(logger) << message << LOG_ASYNC;
			break;
		case ep::Severity::info:
			LOG_INFO(logger) << message << LOG_ASYNC;
			break;
		case ep::Severity::warn:
			LOG_WARN(logger) << message << LOG_ASYNC;
			break;
		case ep::Severity::error:
			LOG_ERROR(logger) << message << LOG_ASYNC;
			break;
		case ep::Severity::fatal:
			LOG_FATAL(logger) << message << LOG_ASYNC;
			break;
		default:
			LOG_ERROR_ASYNC(logger, "Unhandled pool log callback severity");
			LOG_ERROR(logger) << message << LOG_ASYNC;
	}
}

void print_lib_versions(log::Logger& logger) {
	LOG_DEBUG(logger)
		<< "Compiled with library versions: " << "\n"
		<< " - Boost " << BOOST_VERSION / 100000 << "."
		<< BOOST_VERSION / 100 % 1000 << "."
		<< BOOST_VERSION % 100 << "\n"
		<< " - " << Botan::version_string() << "\n"
		<< " - " << drivers::DriverType::name()
		<< " ("  << drivers::DriverType::version() << ")" << "\n"
		<< " - PCRE " << PCRE_MAJOR << "." << PCRE_MINOR << "\n"
		<< " - Zlib " << ZLIB_VERSION << LOG_SYNC;
}

po::options_description Service::options() {
	po::options_description opts;
	opts.add_options()
		("dbc.path", po::value<std::string>()->required())
		("misc.concurrency", po::value<unsigned int>())
		("realm.builds", po::value<std::vector<GameVersion>>()->composing()->required())
		("realm.id", po::value<unsigned int>()->required())
		("realm.max_slots", po::value<unsigned int>()->required())
		("realm.reserved_slots", po::value<unsigned int>()->required())
		("realm.auth_timeout", po::value<unsigned int>()->required())
		("realm.char_list_timeout", po::value<unsigned int>()->required())
		("spark.address", po::value<std::string>()->required())
		("spark.port", po::value<std::uint16_t>()->required())
		("stun.enabled", po::value<bool>()->required())
		("stun.server", po::value<std::string>()->required())
		("stun.port", po::value<std::uint16_t>()->required())
		("stun.protocol", po::value<stun::Protocol>()->required())
		("nsd.host", po::value<std::string>()->required())
		("nsd.port", po::value<std::uint16_t>()->required())
		("forward.enabled", po::value<bool>()->required())
		("forward.method", po::value<ports::Forward::Method>()->required())
		("forward.gateway", po::value<std::string>()->required())
		("network.interface", po::value<std::string>()->required())
		("network.port", po::value<std::uint16_t>()->required())
		("network.tcp_no_delay", po::value<bool>()->required())
		("network.compression", po::value<std::uint8_t>()->required())
		("console_log.enable_input", po::value<bool>()->required())
		("console_log.verbosity", po::value<log::Severity>()->required())
		("console_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("console_log.colours", po::value<bool>()->required())
		("remote_log.verbosity", po::value<log::Severity>()->required())
		("remote_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("remote_log.service_name", po::value<std::string>()->required())
		("remote_log.host", po::value<std::string>()->required())
		("remote_log.port", po::value<std::uint16_t>()->required())
		("file_log.verbosity", po::value<log::Severity>()->required())
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

	return opts;
}

} // gateway, ember