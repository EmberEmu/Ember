/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include "ClientBuilder.h"
#include "DBCRequired.h"
#include "FilterTypes.h"
#include "LoggingCallbacks.h"
#include "ServiceContextImpl.h"
#include "commands/Connections.h"
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
#include <thread/Utility.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/program_options.hpp>
#include <boost/version.hpp>
#include <botan/version.h>
#ifdef WITH_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif
#include <pcre.h>
#include <zlib.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <format>
#include <limits>
#include <memory>
#include <random>
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

Service::Service(log::Logger& logger, commands::Command& registry)
	: logger(logger)
	, registry(registry)
	, stop_flag(0) {}

int Service::run(const opts::variables_map& args) try {
	auto concurrency = args["misc.concurrency"].as<unsigned int>();

	if(!concurrency) {
		concurrency = thread::hardware_concurrency([&](auto msg) {
			SLOG_ERROR(logger, msg);
		});
	}

	SLOG_INFO(logger, "Starting service pool with {} threads", concurrency);
	auto ctx = context.get();
	ctx->service_pool = std::make_unique<thread::ServicePool>(
		concurrency, BOOST_ASIO_CONCURRENCY_HINT_UNSAFE_IO
	);

	std::jthread runner([&] {
		stop_flag.acquire();
		ctx->service_pool->shutdown();
	});

	initialise(args);
	ctx->service_pool->run();
	runner.join();

	SLOG_INFO(logger, "{} stopped", app_name);
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	SLOG_FATAL(logger, e.what());
	return EXIT_FAILURE;
}

void Service::initialise(const opts::variables_map& args) try {
	auto ctx = context.get();
	const auto time = std::chrono::steady_clock::now();

	print_lib_versions(logger);

	auto allowed_builds = args["realm.builds"].as<std::vector<GameVersion>>();
	std::string builds;

	for(const auto& client : allowed_builds) {
		builds += to_string(client) + " ";
	}

	SLOG_INFO(logger, "Allowed client builds: {}", builds);

	auto stun = create_stun_client(args);
	const auto stun_enabled = args["stun.enabled"].as<bool>();
	const auto forward_enabled = args["forward.enabled"].as<bool>();

	std::future<stun::MappedResult> stun_res;

	if(stun_enabled) {
		stun.log_callback([&](const stun::Severity severity, const stun::Error reason) {
			stun_log_callback(severity, reason, logger);
		});

		SLOG_INFO(logger, "Starting STUN query...");
		stun_res = stun.external_address();
	}

	SLOG_INFO(logger, "Seeding xorshift RNG...");
	seed_xorshift_rng();

	SLOG_INFO(logger, "Loading DBC data...");
	dbc::DiskLoader loader(args["dbc.path"].as<std::string>(), [&](auto message) {
		SLOG_DEBUG(logger, message);
	});

	ctx->dbcs = std::make_unique<dbc::Storage>(std::move(loader.load(dbcs_required)));

	SLOG_INFO(logger, "Resolving DBC references...");
	dbc::link(*ctx->dbcs);

	const auto realm_id = args["realm.id"].as<unsigned int>();
	SLOG_INFO(logger, "Loading configuration for realm ID {}", realm_id);

	if(auto realm = load_realm(args, logger)) {
		ctx->realm = std::move(*realm);
	} else {
		throw std::invalid_argument(
			std::format("Configured realm ID {} does not exist in database.", realm_id)
		);
	}

	const auto& title = std::format("{} - {}", app_name, ctx->realm.name);
	utility::set_window_title(title);

	// Validate category & region
	const auto& cat_name = category_name(ctx->realm, ctx->dbcs->cfg_categories);
	SLOG_INFO(logger, "Serving as realm for {} ({})", ctx->realm.name, cat_name);

	SLOG_INFO(logger, "Starting event dispatcher...");
	ctx->dispatcher = std::make_unique<EventDispatcher>(*ctx->service_pool, logger);

	SLOG_INFO(logger, "Starting Spark service...");
	const auto& s_address = args["spark.address"].as<std::string>();
	auto s_port = args["spark.port"].as<std::uint16_t>();

	const auto port = args["network.port"].as<std::uint16_t>();
	const auto& interface = args["network.interface"].as<std::string>();
	const auto tcp_no_delay = args["network.tcp_no_delay"].as<bool>();

	auto update_realm_address = [](Realm& realm) {
		realm.address = std::format("{}:{}", realm.ip, realm.port);
	};

	// If the port specified in the database differs from the config file, use the config file
	if(port != ctx->realm.port) {
		SLOG_WARN(
			logger, "Configured port {} differs from database entry port {}, using {}",
			port, ctx->realm.port, port
		);

		ctx->realm.port = port;
		update_realm_address(ctx->realm);
	}

	// Retrieve STUN result
	if(stun_enabled) {
		SLOG_INFO(logger, "Waiting on STUN result...");

		const auto result = stun_res.get();
		log_stun_result(stun, result, port, logger);

		if(result) {
			ctx->realm.ip = stun::extract_ip_to_string(*result);
			update_realm_address(ctx->realm);
		}
	}

	SLOG_INFO(logger, "Realm will be advertised on {}", ctx->realm.address);

	// Start port forwarding
	auto& service = ctx->service_pool->get(0);

	if(forward_enabled) {
		const auto mode = args["forward.method"].as<ports::Forward::Method>();
		const auto gateway = args["forward.gateway"].as<std::string>();

		ctx->port_daemon = std::make_unique<ports::Forward>(
			service, mode, interface, gateway, port, [&](auto severity, auto message) {
				forward_log_callback(severity, message, logger);
			}
		);
	}
	
	// Load config into store
	const auto config = generate_config(args);
	ctx->config_store = std::make_unique<ConfigStore>(config);
	update_config(config, true);

	SLOG_INFO(logger, "Starting RPC services...");
	ctx->rpc = std::make_unique<spark::Server>(service, app_name, s_address, s_port, logger);
	ctx->rpc_realm = std::make_unique<RealmService>(*ctx->rpc, ctx->realm, logger);
	ctx->rpc_account= std::make_unique<AccountClient>(*ctx->rpc, logger);
	ctx->rpc_character = std::make_unique<CharacterClient>(*ctx->rpc, *ctx->config_store, logger);
	ctx->rpc_world = std::make_unique<WorldRPCClient>(*ctx->rpc, logger);

	const auto& nsd_host = args["nsd.host"].as<std::string>();
	const auto nsd_port = args["nsd.port"].as<std::uint16_t>();

	ctx->rpc_discovery = std::make_unique<NetworkServiceDiscovery>(*ctx->rpc, nsd_host, nsd_port, logger);
	ctx->queue = std::make_unique<RealmQueue>(service, *ctx->dispatcher);

	// Misc. information
	const auto max_socks = utility::max_sockets_desc();
	SLOG_INFO(logger, "Max allowed sockets: {}", max_socks);

	ClientBuilder builder(
		*ctx->config_store, *ctx->dispatcher, *ctx->queue, *ctx->rpc_account, *ctx->rpc_character, logger
	);

	// Start network listener
	SLOG_INFO(logger, "Starting network service on {}:{}...", interface, port);
	ctx->server = std::make_unique<NetworkListener>(
		interface, port, tcp_no_delay, *ctx->service_pool, builder, ctx->sessions, logger
	);
	
	// Start timer service
	ctx->timer = std::make_unique<BroadcastTimer>(
		*ctx->service_pool, *ctx->dispatcher, config::broadcast_timer_frequency
	);

	ctx->timer->start();

	// Install service command handlers
	SLOG_INFO(logger, "Registering command handlers...");
	register_commands(service);

	// All done setting up
	boost::asio::dispatch(service, [&, time, ctx]() {
		ctx->rpc_realm->set_online();

		SLOG_INFO(logger, "{} started successfully in {}", app_name,
			utility::time_elapsed_format(time));

		start_time = std::chrono::steady_clock::now();
	});
} catch(...) {
	stop_flag.release();
	std::rethrow_exception(std::current_exception());
}

void Service::register_commands(boost::asio::io_context& ioc) {
	auto ctx = context.get();

	ctx->cmd_exec = std::make_unique<utility::CommandExecutor>(
		ioc, [&](auto reason) {
			LOG_CONERR(logger, "Command could not be executed, {}", reason);
		}
	);

	auto cmd = registry.scoped_insert(commands::create("config_reload")
		->argument<std::string>("filename", commands::optional)
		->description("Reload the service configuration")
		->handler(ctx->cmd_exec->wrap([&](const commands::Arguments& arguments) {
			std::string config_file;

			if(arguments.contains("filename")) {
				config_file = arguments["filename"].as<std::string>();
			} else {
				config_file = "realm.conf";
			}

			try {
				const auto config = generate_config(reload_options(config_file));
				update_config(config);
				LOG_CONSOLE(logger, "Configuration file successfully reloaded");
			} catch(const std::exception& e) {
				LOG_CONERR(logger, "Could not reload configuration, '{}'", e.what());
			}
		})
	));
	
	auto conn_root = add_connections_commands(
		registry, *ctx->cmd_exec, *ctx->dispatcher, ctx->sessions, logger
	);

	ctx->commands.emplace_back(std::move(conn_root));
	ctx->commands.emplace_back(std::move(cmd));
}

void Service::update_config(const Config& config, bool post_only) {
	auto ctx = context.get();

	if(!post_only) {
		ctx->config_store->update_default_config(config);
	}

	for(auto& service : *ctx->service_pool) {
		boost::asio::dispatch(*service, [&, ctx, config] {
			ctx->config_store->update_thread_config(config);
		});
	}
}

std::string_view category_name(const Realm& realm, const dbc::Store<dbc::Cfg_Categories>& dbc) {
	for(auto& record : dbc | std::views::values) {
		if(record.category == realm.category && record.region == realm.region) {
			return record.name.en_gb;
		}
	}

	throw std::invalid_argument("Unknown category/region combination in database");
}

Config Service::generate_config(const opts::variables_map& args) {
	auto ctx = context.get();

	return Config {
		.realm = ctx->realm,
		.realm_id = ctx->realm.id,
		.max_slots = args["realm.max_slots"].as<unsigned int>(),
		.auth_timeout = std::chrono::seconds(args["realm.auth_timeout"].as<unsigned int>()),
		.char_list_timeout = std::chrono::seconds(args["realm.char_list_timeout"].as<unsigned int>()),
		.allowed_builds = args["realm.builds"].as<std::vector<GameVersion>>()
	};
}

/*
 * Split from launch() as the DB connection is only needed for
 * loading the initial realm information. If the realm requires
 * connections elsewhere in the future, this should be merged back.
 */
std::optional<Realm> load_realm(const opts::variables_map& args, log::Logger& logger) {
	SLOG_INFO(logger, "Initialising database driver...");
	const auto& db_config_path = args["database.config_path"].as<std::string>();
	auto driver(drivers::init_db_driver(db_config_path, "login"));

	SLOG_INFO(logger, "Initialising database connection pool...");

	connection_pool::PoolImpl<drivers::AutoSelect, connection_pool::CheckinClean, connection_pool::ExponentialGrowth> pool(
		std::move(driver), 1, 1, 30s
	);
	
	pool.logging_callback([&](auto severity, auto message) {
		pool_log_callback(severity, message, logger);
	});

	SLOG_INFO(logger, "Initialising DAOs...");
	auto realm_dao = dal::realm_dao(pool);

	SLOG_INFO(logger, "Retrieving realm information...");
	return realm_dao.get_realm(args["realm.id"].as<unsigned int>());
}

void Service::seed_xorshift_rng() {
	constexpr auto bits = std::numeric_limits<std::uint64_t>::digits;
	std::independent_bits_engine<std::default_random_engine, bits, std::uint64_t> engine;
	std::ranges::generate(rng::xorshift::seed, engine);
}

void Service::stop() {
	bool expected = false;

	if(!stopped.compare_exchange_strong(expected, true)) {
		return;
	}

	SLOG_TRACE(logger, "{} shutting down...", app_name);
	auto ctx = context.get();

	// this all needs to be reworked, waiting to switch to coroutines
	boost::asio::post(ctx->service_pool->get(0), [&] {
		auto ctx = context.get();
		ctx->cmd_exec->signal_stop();
		ctx->timer->stop();
		ctx->server->shutdown();
		ctx->rpc_discovery->stop();
		ctx->port_daemon.reset();
		ctx->queue->shutdown();
		ctx->rpc->shutdown();
		stop_flag.release();
	});
}

Service::~Service() {
	stop();
}

void print_lib_versions(log::Logger& logger) {
	LOG_DEBUG_STREAM(logger)
		<< "Compiled with library versions: " << "\n"
		<< " - Boost " << BOOST_VERSION / 100000 << "."
		<< BOOST_VERSION / 100 % 1000 << "."
		<< BOOST_VERSION % 100 << "\n"
		<< " - " << Botan::version_string() << "\n"
		<< " - " << drivers::DriverType::name()
		<< " ("  << drivers::DriverType::version() << ")" << "\n"
		<< " - PCRE " << PCRE_MAJOR << "." << PCRE_MINOR << "\n"
		<< " - Zlib " << ZLIB_VERSION
#ifdef WITH_JEMALLOC
		<< "\n" << " - jemalloc " << JEMALLOC_VERSION
#endif
		<< LOG_SYNC;
}

opts::variables_map Service::reload_options(const std::string& filename) {
	std::ifstream stream(filename);

	if(!stream) {
		throw std::invalid_argument(
			std::format(R"(Unable to open configuration file "{}")", filename)
		);
	}

	opts::variables_map args;
	opts::store(opts::parse_config_file(stream, options(), true), args);
	opts::notify(args);
	return args;
}

opts::options_description Service::options() {
	opts::options_description opts;
	opts.add_options()
		("dbc.path", opts::value<std::string>()->required())
		("misc.concurrency", opts::value<unsigned int>()->required())
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
		("database.config_path", opts::value<std::string>()->required())
		("metrics.enabled", opts::value<bool>()->required())
		("metrics.statsd_host", opts::value<std::string>()->required())
		("metrics.statsd_port", opts::value<std::uint16_t>()->required())
		("monitor.enabled", opts::value<bool>()->required())
		("monitor.interface", opts::value<std::string>()->required())
		("monitor.port", opts::value<std::uint16_t>()->required());

	return opts;
}

extern "C" {

EMBER_EXPORT_SERVICE Service* create_realm(log::Logger& logger, commands::Command& registry) {
	return new Service(logger, registry);
}

EMBER_EXPORT_SERVICE void destroy_realm(Service* service) {
	delete service;
}

} // extern "C"

} // realm, ember