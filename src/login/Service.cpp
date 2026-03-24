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
#include "InitHelpers.h"
#include "MonitorCallbacks.h"
#include "ServiceContextImpl.h"
#include <dbcreader/DiskLoader.h>
#include <logger/Logger.h>
#include <ports/Utility.h>
#include <shared/database/daos/IPBanDAO.h>
#include <shared/database/daos/PatchDAO.h>
#include <shared/database/daos/RealmDAO.h>
#include <shared/game/GameVersion.h>
#include <shared/game/Utility.h>
#include <shared/utility/cstring_view.hpp>
#include <shared/utility/Utility.h>
#include <shared/utility/xoroshiro128plus.h>
#include <shared/utility/STUN.h>
#include <stun/Client.h>
#include <stun/Utility.h>
#include <botan/version.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/version.hpp>
#include <boost/program_options.hpp>
#ifdef WITH_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif
#include <pcre.h>
#include <zlib.h>
#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <random>
#include <ranges>
#include <string>
#include <span>
#include <string_view>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

using namespace std::chrono_literals;
namespace opts = boost::program_options;
namespace ep = ember::connection_pool;

namespace ember::login {

void print_lib_versions(log::Logger& logger);
bool validate_realm(const Realm& realm, const dbc::Store<dbc::Cfg_Categories>& dbc);
void validate_realms(const RealmList& realmlist, log::Logger& logger, const opts::variables_map& args);

Service::Service(log::Logger& logger, commands::Registry& registry)
	: logger(logger)
	, registry(registry)
	, start_time(std::chrono::steady_clock::now())
	, ioc(thread::hardware_concurrency())
	, stopped(false) {}

/*
 * Starts Asio worker threads, blocking until the launch thread exits
 * upon error or signal handling.
 * 
 * io_context will only return once service stop has been requested and
 * the worker threads finish.
 */
int Service::run(const opts::variables_map& args) try {
	initialise(args);

	// Spawn worker threads for Asio
	const auto concurrency = thread::hardware_concurrency([&](auto msg) {
		LOG_ERROR_SYNC(logger, msg);
	});

	std::vector<std::jthread> threads;
	threads.reserve(concurrency);

	for(unsigned int i = 0; i < concurrency; ++i) {
		threads.emplace_back(&boost::asio::io_context::run, &ioc);
		thread::set_name(threads[i], "Asio Worker");
	}

	ioc.run();

	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, e.what());
	return EXIT_FAILURE;
}

void Service::initialise(const opts::variables_map& args) {
	const auto time = std::chrono::steady_clock::now();
	auto ctx = context.get();

	print_lib_versions(logger);

	auto allowed_builds = args["login.builds"].as<std::vector<GameVersion>>();
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
	seed_xorshift_rng();

	const auto concurrency = thread::hardware_concurrency([&](auto msg) {
		LOG_ERROR_SYNC(logger, msg);
	});

	auto max_conns = args["database.max_connections"].as<unsigned short>();

	if(!max_conns) {
		max_conns = concurrency;
	}
	
	LOG_INFO_SYNC(logger, "Max. database connections set to {} ({} cores)", max_conns, concurrency);

	LOG_INFO_SYNC(logger, "Initialising database connection pool...");
	ctx->conn_pool = std::make_unique<connection_pool::Pool<drivers::AutoSelect>>(
		init_database(args, logger)
	);

	LOG_INFO_SYNC(logger, "Initialising DAOs...");
	auto user_dao = dal::user_dao(ctx->conn_pool->get());
	auto patch_dao = dal::patch_dao(ctx->conn_pool->get());
	auto realm_dao = dal::realm_dao(ctx->conn_pool->get());
	auto ip_ban_dao = dal::ip_ban_dao(ctx->conn_pool->get());
	ctx->user_dao = std::make_unique<decltype(user_dao)>(std::move(user_dao));
	ctx->ip_ban_cache = std::make_unique<IPBanCache>(ip_ban_dao.all_bans());

	// Load integrity, patch and survey data
	LOG_INFO_SYNC(logger, "Loading client integrity validation data...");
	ctx->integrity_data = std::make_unique<IntegrityData>();

	if(args["integrity.enabled"].as<bool>()) {
		const auto& bin_path = args["integrity.bin_path"].as<std::string>();
		ctx->integrity_data->add_versions(allowed_builds, bin_path);
	}

	LOG_INFO_SYNC(logger, "Loading patch data...");
	auto patches = Patcher::load_patches(
		args["patches.bin_path"].as<std::string>(), patch_dao
	);

	ctx->patcher = std::make_unique<Patcher>(std::move(allowed_builds), std::move(patches));
	ctx->survey = std::make_unique<Survey>(args["survey.id"].as<std::uint32_t>());

	if(ctx->survey->id()) {
		LOG_INFO_SYNC(logger, "Loading survey data...");

		ctx->survey->add_data(
			grunt::Platform::x86, grunt::System::Win,
		    args["survey.path"].as<std::string>()
		);
	}

	LOG_INFO_SYNC(logger, "Loading realm list...");
	ctx->realm_list = std::make_unique<RealmList>(realm_dao.get_realms());

	const auto realm_count = ctx->realm_list->realms()->size();
	LOG_INFO_SYNC(logger, "Added {} realm{}", realm_count, (!realm_count || realm_count > 1? "s" : ""));

	for(const auto& realm : *ctx->realm_list->realms() | std::views::values) {
		LOG_DEBUG_SYNC(logger, "#{} {}", realm.id, realm.name);
	}

	validate_realms(*ctx->realm_list, logger, args);

	const auto& s_address = args["spark.address"].as<std::string>();
	auto s_port = args["spark.port"].as<std::uint16_t>();

	LOG_INFO_SYNC(logger, "Starting RPC services...");
	ctx->rpc = std::make_unique<spark::Server>(ioc, app_name, s_address, s_port, logger);
	ctx->rpc_account = std::make_unique<AccountClient>(*ctx->rpc, logger);
	ctx->rpc_realm = std::make_unique<RealmClient>(*ctx->rpc, *ctx->realm_list, logger);

	// Start metrics service
	ctx->metrics = start_metrics(ioc, args);

	LOG_INFO_SYNC(logger, "Starting thread pool with {} threads...", concurrency);
	ctx->thread_pool = std::make_unique<thread::ThreadPool>(concurrency);

	const LoginHandler::Options config {
		.locales = args["misc.locales"].as<bool>(),
		.integrity_check = args["integrity.enabled"].as<bool>(),
		.verified_email = args["misc.verified_emails"].as<bool>()
	};

	ctx->login_handler_builder = std::make_unique<LoginHandlerBuilder>(
		logger, *ctx->patcher, *ctx->survey, *ctx->integrity_data, *ctx->user_dao,
	    *ctx->rpc_account, *ctx->realm_list, *ctx->metrics, config
	);

	ctx->login_session_builder = std::make_unique<LoginSessionBuilder>(
		*ctx->login_handler_builder, *ctx->thread_pool
	);

	const auto& interface = args["network.interface"].as<std::string>();
	const auto port = args["network.port"].as<std::uint16_t>();
	const auto tcp_no_delay = args["network.tcp_no_delay"].as<bool>();

	LOG_INFO_SYNC(logger, "Starting network service on {}:{}...", interface, port);
	ctx->server = std::make_unique<NetworkListener>(
		ioc, interface, port, tcp_no_delay, *ctx->login_session_builder,
		*ctx->ip_ban_cache, logger, *ctx->metrics
	);

	// Start monitoring service
	if(args["monitor.enabled"].as<bool>()) {
		LOG_INFO_SYNC(logger, "Starting monitoring service...");

		ctx->monitor = std::make_unique<Monitor>(	
			ioc, args["monitor.interface"].as<std::string>(),
			args["monitor.port"].as<std::uint16_t>()
		);

		install_net_monitor(*ctx->monitor, *ctx->server, logger);
		install_pool_monitor(*ctx->monitor, ctx->conn_pool->get(), logger);
	}

	// Start metrics polling
	ctx->metrics_poll = std::make_unique<MetricsPoll>(ioc, *ctx->metrics);

	ctx->metrics_poll->add_source([&pool = *ctx->conn_pool](Metrics& metrics) {
		metrics.gauge("db_connections", pool->size());
	}, 5s);

	ctx->metrics_poll->add_source([&server = *ctx->server](Metrics& metrics) {
		metrics.gauge("sessions", server.connection_count());
	}, 5s);

	// Misc. information
	LOG_INFO_SYNC(logger, "Max allowed sockets: {}", utility::max_sockets_desc());
	
	// Retrieve STUN result
	if(stun_enabled) {
		LOG_INFO_SYNC(logger, "Waiting on STUN result...");

		const auto result = stun_res.get();
		log_stun_result(stun, result, port, logger);
	}

	// Start port forwarding
	if(forward_enabled) {
		const auto mode = args["forward.method"].as<ports::Forward::Method>();
		const auto gateway = args["forward.gateway"].as<std::string>();

		ctx->port_daemon = std::make_unique<ports::Forward>(
			ioc, mode, interface, gateway, port, [&](auto severity, auto message) {
				forward_log_callback(severity, message, logger);
			}
		);
	}

	// Install service command handlers
	LOG_INFO_SYNC(logger, "Registering command handlers...");
	register_commands();

	// All done setting up
	boost::asio::dispatch(ioc, [this, time]() {
		LOG_INFO_SYNC(logger, "{} started successfully in {}", app_name,
			utility::time_elapsed_format(time));

		start_time = std::chrono::steady_clock::now();
	});
}

void Service::register_commands() {
	auto ctx = context.get();

	ctx->cmd_exec = std::make_unique<utility::CommandExecutor>(ioc, [&](const auto& reason) {
		LOG_CONSOLE_ERROR_ASYNC(logger, "Command could not be executed, {}", reason);
	});
	
	auto handle = registry.scoped_insert(commands::Command::create("connections")
		->description("Display open connection count")
		->handler(ctx->cmd_exec->wrap([this, &server = *ctx->server](auto& command) {
			LOG_CONSOLE_ASYNC(logger, "{} active connection(s), {} peak",
				server.connection_count(), server.peak_connections());
		}))
	);

	ctx->commands.emplace_back(std::move(handle));

	handle = registry.scoped_insert(commands::Command::create("uptime")
		->description("Display service uptime")
		->handler(ctx->cmd_exec->wrap([&](auto& command) {
			const auto uptime = std::chrono::steady_clock::now() - start_time;
			LOG_CONSOLE_ASYNC(logger, "Server has been up for {}", utility::time_duration_format(uptime));
		}))
	);

	ctx->commands.emplace_back(std::move(handle));
}

std::unique_ptr<Metrics> Service::start_metrics(boost::asio::io_context& service, const opts::variables_map& args) {
	auto metrics = std::make_unique<Metrics>();

	if(args["metrics.enabled"].as<bool>()) {
		LOG_INFO_SYNC(logger, "Starting metrics service...");
		metrics = std::make_unique<MetricsImpl>(
			service, args["metrics.statsd_host"].as<std::string>(),
			args["metrics.statsd_port"].as<std::uint16_t>()
		);
	}

	return metrics;
}

void Service::seed_xorshift_rng() {
	constexpr auto bits = std::numeric_limits<std::uint64_t>::digits;
	std::independent_bits_engine<std::default_random_engine, bits, std::uint64_t> engine;
	std::ranges::generate(rng::xorshift::seed, engine);
}

void validate_realms(const RealmList& realmlist, log::Logger& logger, const opts::variables_map& args) {
	LOG_INFO_SYNC(logger, "Loading DBC data...");

	dbc::DiskLoader loader(args["dbc.path"].as<std::string>(), [&](auto message) {
		LOG_DEBUG(logger) << message << LOG_SYNC;
	});

	const auto dbcs = loader.load(dbcs_required);

	for(const auto& realm : *realmlist.realms() | std::views::values) {
		if(!validate_realm(realm, dbcs.cfg_categories)) {
			LOG_WARN_SYNC(logger, "Validation failed for {} - client may not display this realm", realm.name);
		}
	}
}

void Service::stop() {
	bool expected = false;

	if(!stopped.compare_exchange_strong(expected, true)) {
		return;
	}

	LOG_TRACE_SYNC(logger, "Service termination requested");

	boost::asio::post(ioc, [&] {
		auto ctx = context.get();
		ctx->cmd_exec->signal_stop();
		ctx->thread_pool->shutdown();
		context.reset(); // todo, determine proper order
	});
}

Service::~Service() {
	stop();
}

opts::options_description Service::options() {
	opts::options_description opts;
	opts.add_options()
		("dbc.path", opts::value<std::string>()->required())
		("login.builds", opts::value<std::vector<GameVersion>>()->composing()->required())
		("misc.locales", opts::value<bool>()->required())
		("misc.verified_emails", opts::value<bool>()->required())
		("patches.bin_path", opts::value<std::string>()->required())
		("survey.path", opts::value<std::string>()->required())
		("survey.id", opts::value<std::uint32_t>()->required())
		("integrity.enabled", opts::value<bool>()->default_value(false))
		("integrity.bin_path", opts::value<std::string>()->required())
		("spark.address", opts::value<std::string>()->required())
		("spark.port", opts::value<std::uint16_t>()->required())
		("nsd.host", opts::value<std::string>()->required())
		("nsd.port", opts::value<std::uint16_t>()->required())
		("stun.enabled", opts::value<bool>()->required())
		("stun.server", opts::value<std::string>()->required())
		("stun.port", opts::value<std::uint16_t>()->required())
		("stun.protocol", opts::value<stun::Protocol>()->required())
		("forward.enabled", opts::value<bool>()->required())
		("forward.method", opts::value<ports::Forward::Method>()->required())
		("forward.gateway", opts::value<std::string>()->required())
		("network.interface", opts::value<std::string>()->required())
		("network.port", opts::value<std::uint16_t>()->required())
		("network.tcp_no_delay", opts::value<bool>()->default_value(true))
		("database.config_path", opts::value<std::string>()->required())
		("database.min_connections", opts::value<unsigned short>()->required())
		("database.max_connections", opts::value<unsigned short>()->required())
		("metrics.enabled", opts::value<bool>()->required())
		("metrics.statsd_host", opts::value<std::string>()->required())
		("metrics.statsd_port", opts::value<std::uint16_t>()->required())
		("monitor.enabled", opts::value<bool>()->required())
		("monitor.interface", opts::value<std::string>()->required())
		("monitor.port", opts::value<std::uint16_t>()->required());

	return opts;
}

bool validate_realm(const Realm& realm, const dbc::Store<dbc::Cfg_Categories>& dbc) {
	for(auto& record : dbc | std::views::values) {
		if(record.category == realm.category && record.region == realm.region) {
			return true;
		}
	}

	return false;
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
		<< " - Zlib " << ZLIB_VERSION
#ifdef WITH_JEMALLOC
		<< "\n" << " - jemalloc " << JEMALLOC_VERSION
#endif
		<< LOG_SYNC;
}

extern "C" {

EMBER_EXPORT_SERVICE Service* create_login(log::Logger& logger, commands::Registry& registry) {
	return new Service(logger, registry);
}

EMBER_EXPORT_SERVICE void destroy_login(Service* service) {
	delete service;
}

} // extern "C"

} // login, ember