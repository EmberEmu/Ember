/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include "DBCRequired.h"
#include "FilterTypes.h"
#include "Locator.h"
#include "LoggingCallbacks.h"
#include "ServiceContextImpl.h"
#include <conpool/ConnectionPool.h>
#include <conpool/Policies.h>
#include <conpool/drivers/AutoSelect.h>
#include <dbcreader/Reader.h>
#include <logger/Logger.h>
#include <nsd/NSD.h>
#include <ports/Utility.h>
#include <shared/utility/EnumHelper.h>
#include <shared/database/daos/RealmDAO.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/game/Utility.h>
#include <shared/utility/cstring_view.hpp>
#include <shared/utility/LogConfig.h>
#include <shared/utility/STUN.h>
#include <shared/utility/Utility.h>
#include <shared/utility/xoroshiro128plus.h>
#include <stun/Client.h>
#include <stun/Utility.h>
#include <thread/ServicePool.h>
#include <thread/Utility.h>
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

namespace opts = boost::program_options;

namespace ember::realm {

std::optional<Realm> load_realm(const opts::variables_map& args, log::Logger& logger);
std::string_view category_name(const Realm& realm, const dbc::Store<dbc::Cfg_Categories>& dbc);
void print_lib_versions(log::Logger& logger);

Service::Service(log::Logger& logger, commands::Registry& registry)
	: logger(logger)
	, registry(registry)
	, stop_flag(0) {}

int Service::run(const opts::variables_map& args) try {
	const auto concurrency = thread::hardware_concurrency([&](auto msg) {
		LOG_ERROR_SYNC(logger, "{}", msg);
	});

	LOG_INFO_SYNC(logger, "Starting service pool with {} threads", concurrency);
	thread::ServicePool service_pool(concurrency, BOOST_ASIO_CONCURRENCY_HINT_UNSAFE_IO);
	initialise(args, service_pool);

	std::jthread runner([&] {
		service_pool.run();
		stop_flag.acquire();
		service_pool.stop();
	});

	runner.join();
	LOG_TRACE_SYNC(logger, "{} terminated...", app_name);
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, "{}", e.what());
	return EXIT_FAILURE;
}

void Service::initialise(const opts::variables_map& args, thread::ServicePool& service_pool) {
	auto ctx = context.get();
	const auto time = std::chrono::steady_clock::now();

	print_lib_versions(logger);

	auto allowed_builds = args["realm.builds"].as<std::vector<GameVersion>>();
	std::string builds;

	for(const auto& client : allowed_builds) {
		builds += to_string(client) + " ";
	}

	LOG_INFO_SYNC(logger, "Allowed client builds: {}", builds);

	auto stun = create_stun_client(args);
	const auto stun_enabled = args["stun.enabled"].as<bool>();
	const auto forward_enabled = args["forward.enabled"].as<bool>();

	std::future<stun::MappedResult> stun_res;

	if(stun_enabled) {
		stun.log_callback([&](const stun::Severity severity, const stun::Error reason) {
			stun_log_callback(severity, reason, logger);
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

	ctx->dbcs = std::make_unique<dbc::Storage>(std::move(loader.load(dbcs_required)));

	LOG_INFO_SYNC(logger, "Resolving DBC references...");
	dbc::link(*ctx->dbcs);

	const auto realm_id = args["realm.id"].as<unsigned int>();
	LOG_INFO_SYNC(logger, "Loading configuration for realm ID {}", realm_id);

	if(auto realm = load_realm(args, logger)) {
		ctx->realm = std::make_unique<Realm>(std::move(*realm));
	} else {
		throw std::invalid_argument(
			std::format("Configured realm ID {} does not exist in database.", realm_id)
		);
	}

	const auto& title = std::format("{} - {}", app_name, ctx->realm->name);
	utility::set_window_title(title);

	// Validate category & region
	const auto& cat_name = category_name(*ctx->realm, ctx->dbcs->cfg_categories);
	LOG_INFO_SYNC(logger, "Serving as realm for {} ({})", ctx->realm->name, cat_name);

	LOG_INFO_SYNC(logger, "Starting event dispatcher...");
	ctx->dispatcher = std::make_unique<EventDispatcher>(service_pool, logger);

	LOG_INFO_SYNC(logger, "Starting Spark service...");
	const auto& s_address = args["spark.address"].as<std::string>();
	auto s_port = args["spark.port"].as<std::uint16_t>();

	const auto port = args["network.port"].as<std::uint16_t>();
	const auto& interface = args["network.interface"].as<std::string>();
	const auto tcp_no_delay = args["network.tcp_no_delay"].as<bool>();

	auto update_realm_address = [](Realm& realm) {
		realm.address = std::format("{}:{}", realm.ip, realm.port);
	};

	// If the database port differs from the config file port, use the config file port
	if(port != ctx->realm->port) {
		LOG_WARN_SYNC(
			logger, "Configured port {} differs from database entry port {}, using {}",
			port, ctx->realm->port, port
		);

		ctx->realm->port = port;
		update_realm_address(*ctx->realm);
	}

	// Retrieve STUN result
	if(stun_enabled) {
		LOG_INFO_SYNC(logger, "Waiting on STUN result...");

		const auto result = stun_res.get();
		log_stun_result(stun, result, port, logger);

		if(result) {
			ctx->realm->ip = stun::extract_ip_to_string(*result);
			update_realm_address(*ctx->realm);
		}
	}

	// Start port forwarding
	auto& service = service_pool.get();

	if(forward_enabled) {
		const auto mode = args["forward.method"].as<ports::Forward::Method>();
		const auto gateway = args["forward.gateway"].as<std::string>();

		ctx->port_daemon = std::make_unique<ports::Forward>(
			service, mode, interface, gateway, port, [&](auto severity, auto message) {
				forward_log_callback(severity, message, logger);
			}
		);
	}
	
	ctx->config = std::make_unique<Config>(Config {
		.realm = *ctx->realm,
		.realm_id = realm_id,
		.max_slots = args["realm.max_slots"].as<unsigned int>(),
		.auth_timeout = std::chrono::seconds(args["realm.auth_timeout"].as<unsigned int>()),
		.char_list_timeout = std::chrono::seconds(args["realm.char_list_timeout"].as<unsigned int>()),
		.allowed_builds = std::move(allowed_builds)
	});

	LOG_INFO_SYNC(logger, "Realm will be advertised on {}", ctx->realm->address);

	ctx->queue = std::make_unique<RealmQueue>(service_pool.get());
	
	LOG_INFO_SYNC(logger, "Starting RPC services...");
	ctx->rpc = std::make_unique<spark::Server>(service_pool.get(), app_name, s_address, s_port, logger);
	ctx->rpc_realm = std::make_unique<RealmService>(*ctx->rpc, *ctx->realm, logger);
	ctx->rpc_account= std::make_unique<AccountClient>(*ctx->rpc, logger);
	ctx->rpc_character = std::make_unique<CharacterClient>(*ctx->rpc, *ctx->config, logger);
	ctx->rpc_world = std::make_unique<WorldRPCClient>(*ctx->rpc, logger);

	const auto& nsd_host = args["nsd.host"].as<std::string>();
	const auto nsd_port = args["nsd.port"].as<std::uint16_t>();

	ctx->rpc_discovery = std::make_unique<NetworkServiceDiscovery>(*ctx->rpc, nsd_host, nsd_port, logger);

	// set services - not the best design pattern but it'll do for now
	// todo, this can probably be removed now
	Locator::set(ctx->dispatcher.get());
	Locator::set(ctx->queue.get());
	Locator::set(ctx->rpc_account.get());
	Locator::set(ctx->rpc_character.get());
	Locator::set(ctx->rpc_realm.get());
	Locator::set(ctx->config.get());
	
	// Misc. information
	const auto max_socks = utility::max_sockets_desc();
	LOG_INFO_SYNC(logger, "Max allowed sockets: {}", max_socks);

	// Start network listener
	LOG_INFO_SYNC(logger, "Starting network service...");
	ctx->server = std::make_unique<NetworkListener>(service_pool, interface, port, tcp_no_delay, logger);
	LOG_INFO_SYNC(logger, "Started network service on {}:{}", interface, ctx->server->port());

	// All done setting up
	boost::asio::dispatch(service, [&, time]() {
		auto ctx = context.get();
		ctx->rpc_realm->set_online();

		LOG_INFO_SYNC(logger, "{} started successfully in {}", app_name,
			utility::start_time_format(time));

		start_time = std::chrono::steady_clock::now();
	});
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
 * loading the initial realm information. If the realm requires
 * connections elsewhere in the future, this should be merged back.
 */
std::optional<Realm> load_realm(const opts::variables_map& args, log::Logger& logger) {
	LOG_INFO_SYNC(logger, "Initialising database driver...");
	const auto& db_config_path = args["database.config_path"].as<std::string>();
	auto driver(drivers::init_db_driver(db_config_path, "login"));

	LOG_INFO_SYNC(logger, "Initialising database connection pool...");

	connection_pool::PoolImpl<drivers::AutoSelect, connection_pool::CheckinClean, connection_pool::ExponentialGrowth> pool(
		std::move(driver), 1, 1, 30s
	);
	
	pool.logging_callback([&](auto severity, auto message) {
		pool_log_callback(severity, message, logger);
	});

	LOG_INFO_SYNC(logger, "Initialising DAOs...");
	auto realm_dao = dal::realm_dao(pool);

	LOG_INFO_SYNC(logger, "Retrieving realm information...");
	return realm_dao.get_realm(args["realm.id"].as<unsigned int>());
}

void Service::stop() {
	LOG_INFO_SYNC(logger, "{} shutting down...", app_name);
	context.reset();
	stop_flag.release();
}

Service::~Service() {
	stop();
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

opts::options_description Service::options() {
	opts::options_description opts;
	opts.add_options()
		("dbc.path", opts::value<std::string>()->required())
		("misc.concurrency", opts::value<unsigned int>())
		("realm.builds", opts::value<std::vector<GameVersion>>()->composing()->required())
		("realm.id", opts::value<unsigned int>()->required())
		("realm.max_slots", opts::value<unsigned int>()->required())
		("realm.reserved_slots", opts::value<unsigned int>()->required())
		("realm.auth_timeout", opts::value<unsigned int>()->required())
		("realm.char_list_timeout", opts::value<unsigned int>()->required())
		("spark.address", opts::value<std::string>()->required())
		("spark.port", opts::value<std::uint16_t>()->required())
		("stun.enabled", opts::value<bool>()->required())
		("stun.server", opts::value<std::string>()->required())
		("stun.port", opts::value<std::uint16_t>()->required())
		("stun.protocol", opts::value<stun::Protocol>()->required())
		("nsd.host", opts::value<std::string>()->required())
		("nsd.port", opts::value<std::uint16_t>()->required())
		("forward.enabled", opts::value<bool>()->required())
		("forward.method", opts::value<ports::Forward::Method>()->required())
		("forward.gateway", opts::value<std::string>()->required())
		("network.interface", opts::value<std::string>()->required())
		("network.port", opts::value<std::uint16_t>()->required())
		("network.tcp_no_delay", opts::value<bool>()->required())
		("network.compression", opts::value<std::uint8_t>()->required())
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
		("file_log.path", opts::value<std::string>()->default_value("realm.log"))
		("file_log.timestamp_format", opts::value<std::string>())
		("file_log.mode", opts::value<std::string>()->required())
		("file_log.size_rotate", opts::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", opts::value<bool>()->required())
		("file_log.log_timestamp", opts::value<bool>()->required())
		("file_log.log_severity", opts::value<bool>()->required())
		("database.config_path", opts::value<std::string>()->required())
		("metrics.enabled", opts::value<bool>()->required())
		("metrics.statsd_host", opts::value<std::string>()->required())
		("metrics.statsd_port", opts::value<std::uint16_t>()->required())
		("monitor.enabled", opts::value<bool>()->required())
		("monitor.interface", opts::value<std::string>()->required())
		("monitor.port", opts::value<std::uint16_t>()->required());

	return opts;
}

} // realm, ember