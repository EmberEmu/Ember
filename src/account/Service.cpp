/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include "AccountService.h"
#include "AccountHandler.h"
#include "FilterTypes.h"
#include "Sessions.h"
#include "LoggingCallbacks.h"
#include <logger/Logger.h>
#include <conpool/ConnectionPool.h>
#include <conpool/Policies.h>
#include <conpool/drivers/AutoSelect.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/metrics/MetricsImpl.h>
#include <shared/metrics/Monitor.h>
#include <thread/ThreadPool.h>
#include <thread/Utility.h>
#include <shared/utility/Utility.h>
#include <spark/Server.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <string_view>
#include <thread>
#include <cstddef>
#include <cstdlib>
#include <cstdint>

namespace po = boost::program_options;

namespace ember::account {

/*
 * Starts Asio worker threads, blocking until the launch thread exits
 * upon error or signal handling.
 * 
 * io_context is only stopped after the thread joins to ensure that all
 * services can cleanly shut down upon destruction without requiring
 * explicit shutdown() calls in a signal handler.
 */
int Service::run(const po::variables_map& args) try {
	boost::asio::io_context service(BOOST_ASIO_CONCURRENCY_HINT_UNSAFE_IO);
	auto work = boost::asio::make_work_guard(service);

	std::thread thread([&]() {
		thread::set_name("Launcher");
		launch(args, service);
	});

	std::jthread worker(static_cast<std::size_t(boost::asio::io_context::*)()>
		(&boost::asio::io_context::run), &service);
	thread::set_name(worker, "Asio Worker");

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

void Service::launch(const po::variables_map& args, boost::asio::io_context& service) try {
	constexpr auto concurrency = 1u; // temp
	LOG_INFO_SYNC(logger, "Starting thread pool with {} threads...", concurrency);
	thread::ThreadPool thread_pool(concurrency);

	LOG_INFO_SYNC(logger, "Initialising database driver...");
	const auto& db_config_path = args["database.config_path"].as<std::string>();
	auto driver(drivers::init_db_driver(db_config_path, "login"));
	auto min_conns = args["database.min_connections"].as<unsigned short>();
	auto max_conns = args["database.max_connections"].as<unsigned short>();

	LOG_INFO_SYNC(logger, "Initialising database connection pool...");

	connection_pool::Pool<decltype(driver), connection_pool::CheckinClean, connection_pool::ExponentialGrowth> pool(
		driver, min_conns, max_conns, 30s
	);

	pool.logging_callback([&](auto severity, auto message) {
		pool_log_callback(severity, message, logger);
	});

	LOG_INFO_SYNC(logger,"Initialising DAOs...");
	auto user_dao = dal::user_dao(pool);

	LOG_INFO_SYNC(logger, "Initialising account handler..."); 
	AccountHandler handler(user_dao, thread_pool);

	LOG_INFO_SYNC(logger, "Starting RPC services...");
	const auto& s_address = args["spark.address"].as<std::string>();
	auto s_port = args["spark.port"].as<std::uint16_t>();

	Sessions sessions(true);

	spark::Server spark(service, "account", s_address, s_port, logger);
	AccountService acct_service(spark, handler, sessions, logger);

	// All done setting up
	boost::asio::dispatch(service, [&]() {
		LOG_INFO_SYNC(logger, "{} started successfully in {}", APP_NAME,
			utility::start_time_format(start_time));
	});

	stop_flag.acquire();

	LOG_INFO_SYNC(logger, "{} shutting down...", APP_NAME);
} catch(...) {
	eptr = std::current_exception();
}

void Service::stop() {
	stop_flag.release();
}

po::options_description Service::options() {
	po::options_description opts;
	opts.add_options()
		("spark.address,", po::value<std::string>()->required())
		("spark.port", po::value<std::uint16_t>()->required())
		("nsd.host", po::value<std::string>()->required())
		("nsd.port", po::value<std::uint16_t>()->required())
		("console_log.enable_input", po::value<bool>()->required())
		("console_log.verbosity", po::value<log::Severity>()->required())
		("console_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("console_log.colours", po::bool_switch()->required())
		("remote_log.verbosity", po::value<log::Severity>()->required())
		("remote_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("remote_log.service_name", po::value<std::string>()->required())
		("remote_log.host", po::value<std::string>()->required())
		("remote_log.port", po::value<std::uint16_t>()->required())
		("file_log.verbosity", po::value<log::Severity>()->required())
		("file_log.filter-mask", po::value<std::uint32_t>()->default_value(0))
		("file_log.path", po::value<std::string>()->default_value("account.log"))
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
	return opts;
}

} // account, ember