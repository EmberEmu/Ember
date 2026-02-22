/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include "AccountClient.h"
#include "FilterTypes.h"
#include "IntegrityData.h"
#include "LoggingCallbacks.h"
#include "LoginHandlerBuilder.h"
#include "MonitorCallbacks.h"
#include "NetworkListener.h"
#include "Patcher.h"
#include "RealmClient.h"
#include "RealmList.h"
#include "SessionBuilders.h"
#include "Survey.h"
#include <logger/Logger.h>
#include <conpool/ConnectionPool.h>
#include <conpool/Policies.h>
#include <conpool/drivers/AutoSelect.h>
#include <ports/Forward.h>
#include <ports/Utility.h>
#include <shared/metrics/MetricsImpl.h>
#include <shared/metrics/MetricsPoll.h>
#include <shared/metrics/Monitor.h>
#include <shared/threading/ThreadPool.h>
#include <shared/threading/Utility.h>
#include <shared/database/daos/IPBanDAO.h>
#include <shared/database/daos/PatchDAO.h>
#include <shared/database/daos/RealmDAO.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/game/GameVersion.h>
#include <shared/game/Utility.h>
#include <shared/IPBanCache.h>
#include <shared/utility/cstring_view.hpp>
#include <shared/utility/Utility.h>
#include <shared/utility/xoroshiro128plus.h>
#include <shared/utility/STUN.h>
#include <spark/Server.h>
#include <stun/Client.h>
#include <stun/Utility.h>
#include <botan/version.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/version.hpp>
#include <boost/program_options.hpp>
#include <pcre.h>
#include <zlib.h>
#include <functional>
#include <memory>
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
namespace po = boost::program_options;
namespace ep = ember::connection_pool;

#if defined TARGET_WORKER_COUNT
constexpr std::size_t WORKER_NUM_HINT = TARGET_WORKER_COUNT;
#else
constexpr std::size_t WORKER_NUM_HINT = 32;
#endif

namespace ember::login {

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
	boost::asio::io_context service(concurrency);
	auto work = boost::asio::make_work_guard(service);

	std::thread thread([&]() {
		thread::set_name("Launcher");
		launch(args, service);
	});

	// Spawn worker threads for Asio
	boost::container::small_vector<std::jthread, WORKER_NUM_HINT> workers;
	workers.reserve(concurrency);

	for(unsigned int i = 0; i < concurrency; ++i) {
		workers.emplace_back(static_cast<std::size_t(boost::asio::io_context::*)()>
							 (&boost::asio::io_context::run), &service);
		thread::set_name(workers[i], "Asio Worker");
	}

	thread.join();
	service.stop();

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

void Service::launch(const po::variables_map& args, boost::asio::io_context& service) try {
#ifdef DEBUG_NO_THREADS
	LOG_WARN_SYNC(logger, "Compiled with DEBUG_NO_THREADS!");
#endif

	print_lib_versions(logger);

	const auto allowed_builds = args["login.builds"].as<std::vector<GameVersion>>();
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

	LOG_INFO_SYNC(logger, "Initialising database driver...");
	const auto& db_config_path = args["database.config_path"].as<std::string>();
	auto driver(drivers::init_db_driver(db_config_path, "login"));
	auto min_conns = args["database.min_connections"].as<unsigned short>();
	auto max_conns = args["database.max_connections"].as<unsigned short>();

	unsigned int concurrency = thread::hardware_concurrency(logger);

	if(!max_conns) {
		max_conns = concurrency;
	} else if(max_conns != concurrency) {
		LOG_WARN_SYNC(logger, "Max. database connection count may be non-optimal "
		                      "(use {} to match logical core count)", concurrency);
	}

	LOG_INFO_SYNC(logger, "Initialising database connection pool...");
	ep::Pool<decltype(driver), ep::CheckinClean, ep::ExponentialGrowth> pool(
		driver, min_conns, max_conns, 30s
	);

	pool.logging_callback([&](auto severity, auto message) {
		pool_log_callback(severity, message, logger);
	});

	LOG_INFO_SYNC(logger, "Initialising DAOs...");
	auto user_dao = dal::user_dao(pool);
	auto realm_dao = dal::realm_dao(pool);
	auto patch_dao = dal::patch_dao(pool);
	auto ip_ban_dao = dal::ip_ban_dao(pool); 
	auto ip_ban_cache = IPBanCache(ip_ban_dao.all_bans());

	// Load integrity, patch and survey data
	LOG_INFO_SYNC(logger, "Loading client integrity validation data...");
	IntegrityData bin_data;

	if(args["integrity.enabled"].as<bool>()) {
		const auto& bin_path = args["integrity.bin_path"].as<std::string>();
		bin_data.add_versions(allowed_builds, bin_path);
	}

	LOG_INFO_SYNC(logger, "Loading patch data...");

	auto patches = Patcher::load_patches(
		args["patches.bin_path"].as<std::string>(), patch_dao
	);

	Patcher patcher(allowed_builds, patches);
	Survey survey(args["survey.id"].as<std::uint32_t>());

	if(survey.id()) {
		LOG_INFO_SYNC(logger, "Loading survey data...");

		survey.add_data(grunt::Platform::x86, grunt::System::Win,
		                args["survey.path"].as<std::string>());
	}

	LOG_INFO_SYNC(logger, "Loading realm list...");
	RealmList realm_list(realm_dao.get_realms());

	LOG_INFO_SYNC(logger, "Added {} realm(s)", realm_list.realms()->size());

	for(const auto& realm : *realm_list.realms() | std::views::values) {
		LOG_DEBUG_SYNC(logger, "#{} {}", realm.id, realm.name);
	}

	const auto& s_address = args["spark.address"].as<std::string>();
	auto s_port = args["spark.port"].as<std::uint16_t>();

	LOG_INFO_SYNC(logger, "Starting RPC services...");
	spark::Server spark(service, "login", s_address, s_port, logger);
	AccountClient acct_svc(spark, logger);
	RealmClient realm_svcv2(spark, realm_list, logger);

	// Start metrics service
	auto metrics = start_metrics(service, args);

	LOG_INFO_SYNC(logger, "Starting thread pool with {} threads...", concurrency);
	ThreadPool thread_pool(concurrency);

	LoginHandlerBuilder builder(logger, patcher, survey, bin_data, user_dao,
	                            acct_svc, realm_list, *metrics,
	                            args["misc.locale_enforce"].as<bool>(),
	                            args["integrity.enabled"].as<bool>(),
	                            args["misc.verified_emails"].as<bool>());
	LoginSessionBuilder s_builder(builder, thread_pool);

	const auto& interface = args["network.interface"].as<std::string>();
	const auto port = args["network.port"].as<std::uint16_t>();
	const auto tcp_no_delay = args["network.tcp_no_delay"].as<bool>();

	LOG_INFO_SYNC(logger, "Starting network service...");

	NetworkListener server(
		service, interface, port, tcp_no_delay, s_builder, ip_ban_cache, logger, *metrics
	);

	LOG_INFO_SYNC(logger, "Started network service on {}:{}", interface, server.port());

	// Start monitoring service
	std::unique_ptr<Monitor> monitor;

	if(args["monitor.enabled"].as<bool>()) {
		LOG_INFO_SYNC(logger, "Starting monitoring service...");

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
	LOG_INFO_SYNC(logger, "Max allowed sockets: {}", utility::max_sockets_desc());
	
	// Retrieve STUN result and start port forwarding if enabled and STUN succeeded
	std::unique_ptr<ports::Forward> forward;

	if(stun_enabled) {
		const auto result = stun_res.get();
		log_stun_result(stun, result, port, logger);

		if(result && forward_enabled) {
			const auto nat = stun.nat_present().get();

			if(nat && !*nat) {
				LOG_WARN_SYNC(logger, "Port forwarding skipped as we do not appear to be behind NAT");
			} else {
				const auto& mode = args["forward.method"].as<ports::Forward::Method>();
				const auto& gateway = args["forward.gateway"].as<std::string>();

				forward = std::make_unique<ports::Forward>(
					service, mode, interface, gateway, port, [&](auto severity, auto message) {
						forward_log_callback(severity, message, logger);
					}
				);
			}
		} else if(!result && forward_enabled) {
			LOG_WARN_SYNC(logger, "Port forwarding skipped due to STUN failure");
		}
	}

	// Install service command handlers
	cmd_register("connections")
		->description("Display open connection count")
		->handler([&](auto& command) {
			LOG_CONSOLE_ASYNC(logger, "{} active connection(s), {} peak",
				server.connection_count(), server.peak_connections());
		});

	cmd_register("uptime")
		->description("Display service uptime")
		->handler([&](auto& command) {
			const auto uptime = std::chrono::steady_clock::now() - start_time;
			LOG_CONSOLE_ASYNC(logger, "Server has been up for {}", utility::time_duration_format(uptime));
		});

	// All done setting up
	boost::asio::dispatch(service, [&]() {
		LOG_INFO_SYNC(logger, "{} started successfully in {}", APP_NAME,
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start_time)
		);
	});
	
	stop_flag.acquire();
	LOG_INFO_SYNC(logger, "{} shutting down...", APP_NAME);
} catch(...) {
	eptr = std::current_exception();
}

std::unique_ptr<Metrics> Service::start_metrics(boost::asio::io_context& service, const po::variables_map& args) {
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

po::options_description Service::options() {
	po::options_description opts;
	opts.add_options()
		("login.builds", po::value<std::vector<GameVersion>>()->composing()->required())
		("misc.locale_enforce", po::value<bool>()->required())
		("misc.verified_emails", po::value<bool>()->required())
		("patches.bin_path", po::value<std::string>()->required())
		("survey.path", po::value<std::string>()->required())
		("survey.id", po::value<std::uint32_t>()->required())
		("integrity.enabled", po::value<bool>()->default_value(false))
		("integrity.bin_path", po::value<std::string>()->required())
		("spark.address", po::value<std::string>()->required())
		("spark.port", po::value<std::uint16_t>()->required())
		("nsd.host", po::value<std::string>()->required())
		("nsd.port", po::value<std::uint16_t>()->required())
		("stun.enabled", po::value<bool>()->required())
		("stun.server", po::value<std::string>()->required())
		("stun.port", po::value<std::uint16_t>()->required())
		("stun.protocol", po::value<stun::Protocol>()->required())
		("forward.enabled", po::value<bool>()->required())
		("forward.method", po::value<ports::Forward::Method>()->required())
		("forward.gateway", po::value<std::string>()->required())
		("network.interface", po::value<std::string>()->required())
		("network.port", po::value<std::uint16_t>()->required())
		("network.tcp_no_delay", po::value<bool>()->default_value(true))
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

	return opts;
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

} // login, ember