/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include "ServiceContextImpl.h"
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
#include <type_traits>
#include <cstddef>
#include <cstdlib>
#include <cstdint>

namespace opts = boost::program_options;

namespace ember::account {

connection_pool::Pool<drivers::DriverType> init_database(
	const boost::program_options::variables_map& args, log::Logger& logger
);

Service::Service(log::Logger& logger, commands::PrefixedRegistry& cmd_register)
	: logger(logger)
	, cmd_register(cmd_register)
	, start_time(std::chrono::steady_clock::now())
	, service(BOOST_ASIO_CONCURRENCY_HINT_UNSAFE_IO) {}

int Service::run(const opts::variables_map& args) try {
	initialise(args, service);
	service.run();

	LOG_INFO_SYNC(logger, "{} shutting down...", APP_NAME);
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	LOG_FATAL(logger) << e.what() << LOG_SYNC;
	return EXIT_FAILURE;
}

void Service::initialise(const opts::variables_map& args, boost::asio::io_context& service) {
	auto ctx = context.get();
	
	constexpr auto concurrency = 1u; // temp
	LOG_INFO_SYNC(logger, "Starting thread pool with {} threads...", concurrency);
	ctx->thread_pool = std::make_unique<thread::ThreadPool>(
		concurrency
	);

	LOG_INFO_SYNC(logger, "Initialising database driver...");
	ctx->conn_pool = std::make_unique<connection_pool::Pool<drivers::DriverType>>(
		init_database(args, logger)
	);

	LOG_INFO_SYNC(logger,"Initialising DAOs...");
	auto user_dao = dal::user_dao(*ctx->conn_pool);
	ctx->user_dao = std::make_unique<decltype(user_dao)>(std::move(user_dao));

	LOG_INFO_SYNC(logger, "Initialising account handler..."); 
	ctx->account_handler = std::make_unique<AccountHandler>(
		*ctx->user_dao, *ctx->thread_pool
	);

	LOG_INFO_SYNC(logger, "Starting RPC services...");
	ctx->sessions = std::make_unique<Sessions>(true);

	const auto& s_address = args["spark.address"].as<std::string>();
	auto s_port = args["spark.port"].as<std::uint16_t>();

	ctx->spark = std::make_unique<spark::Server>(
		service, "account", s_address, s_port, logger
	);

	ctx->account_service = std::make_unique<AccountService>(
		*ctx->spark, *ctx->account_handler, *ctx->sessions, logger
	);

	// All done setting up
	boost::asio::dispatch(service, [&]() {
		LOG_INFO_SYNC(logger, "{} started successfully in {}", APP_NAME,
			utility::start_time_format(start_time));
	});
}

void Service::stop() {
	LOG_TRACE_SYNC(logger, "Service termination requested");
	context.reset();
}

connection_pool::Pool<drivers::DriverType> init_database(const opts::variables_map& args, log::Logger& logger) {
	using namespace connection_pool;

	const auto& db_config_path = args["database.config_path"].as<std::string>();
	auto driver(drivers::init_db_driver(db_config_path, "login"));
	auto min_conns = args["database.min_connections"].as<unsigned short>();
	auto max_conns = args["database.max_connections"].as<unsigned short>();

	LOG_INFO_SYNC(logger, "Initialising database connection pool...");

	auto pool = create<decltype(driver), CheckinClean, ExponentialGrowth>(
		std::move(driver), min_conns, max_conns, 30s
	);

	pool->logging_callback([&](auto severity, auto message) {
		pool_log_callback(severity, message, logger);
	});

	return pool;
}

opts::options_description Service::options() {
	opts::options_description opts;
	opts.add_options()
		("spark.address,", opts::value<std::string>()->required())
		("spark.port", opts::value<std::uint16_t>()->required())
		("nsd.host", opts::value<std::string>()->required())
		("nsd.port", opts::value<std::uint16_t>()->required())
		("console_log.enable_input", opts::value<bool>()->required())
		("console_log.verbosity", opts::value<log::Severity>()->required())
		("console_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("console_log.colours", opts::bool_switch()->required())
		("remote_log.verbosity", opts::value<log::Severity>()->required())
		("remote_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("remote_log.service_name", opts::value<std::string>()->required())
		("remote_log.host", opts::value<std::string>()->required())
		("remote_log.port", opts::value<std::uint16_t>()->required())
		("file_log.verbosity", opts::value<log::Severity>()->required())
		("file_log.filter-mask", opts::value<std::uint32_t>()->default_value(0))
		("file_log.path", opts::value<std::string>()->default_value("account.log"))
		("file_log.timestamp_format", opts::value<std::string>())
		("file_log.mode", opts::value<std::string>()->required())
		("file_log.size_rotate", opts::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", opts::bool_switch()->required())
		("file_log.log_timestamp", opts::bool_switch()->required())
		("file_log.log_severity", opts::bool_switch()->required())
		("database.config_path", opts::value<std::string>()->required())
		("database.min_connections", opts::value<unsigned short>()->required())
		("database.max_connections", opts::value<unsigned short>()->required())
		("metrics.enabled", opts::bool_switch()->required())
		("metrics.statsd_host", opts::value<std::string>()->required())
		("metrics.statsd_port", opts::value<std::uint16_t>()->required())
		("monitor.enabled", opts::bool_switch()->required())
		("monitor.interface", opts::value<std::string>()->required())
		("monitor.port", opts::value<std::uint16_t>()->required());
	return opts;
}

} // account, ember