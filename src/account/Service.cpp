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
#include "InitHelpers.h"
#include "Sessions.h"
#include <logger/Logger.h>
#include <shared/database/daos/UserDAO.h>
#include <shared/metrics/MetricsImpl.h>
#include <shared/metrics/Monitor.h>
#include <thread/ThreadPool.h>
#include <thread/Utility.h>
#include <shared/utility/Utility.h>
#include <spark/Server.h>
#include <boost/asio/dispatch.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstdint>

namespace opts = boost::program_options;

namespace ember::account {

Service::Service(log::Logger& logger, commands::Registry& registry)
	: logger(logger)
	, registry(registry)
	, service(BOOST_ASIO_CONCURRENCY_HINT_UNSAFE_IO) {}

int Service::run(const opts::variables_map& args) try {
	initialise(args);
	service.run();
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, e.what());
	return EXIT_FAILURE;
}

void Service::initialise(const opts::variables_map& args) {
	const auto time = std::chrono::steady_clock::now();
	auto ctx = context.get();
	
	LOG_INFO_SYNC(logger, "Initialising database connection pool...");
	ctx->conn_pool = std::make_unique<connection_pool::Pool<drivers::AutoSelect>>(
		init_database(args, logger)
	);

	LOG_INFO_SYNC(logger,"Initialising DAOs...");
	auto user_dao = dal::user_dao(ctx->conn_pool->get());
	ctx->user_dao = std::make_unique<decltype(user_dao)>(std::move(user_dao));

	const auto concurrency = thread::hardware_concurrency([&](auto msg) {
		LOG_ERROR_SYNC(logger, msg);
	});

	LOG_INFO_SYNC(logger, "Starting thread pool with {} threads...", concurrency);
	ctx->thread_pool = std::make_unique<thread::ThreadPool>(
		concurrency
	);

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
	boost::asio::dispatch(service, [&, time]() {
		LOG_INFO_SYNC(logger, "{} started successfully in {}", app_name,
			utility::start_time_format(time));

		start_time = std::chrono::steady_clock::now();
	});
}

void Service::stop() {
	LOG_TRACE_SYNC(logger, "Service termination requested");
	auto ctx = context.get();
	ctx->thread_pool->shutdown();
	ctx->conn_pool->get().close();
}

Service::~Service() {
	stop();
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

extern "C" {

EMBER_EXPORT_SERVICE Service* create_account(log::Logger& logger, commands::Registry& registry) {
	return Service::create(logger, registry).release();
}

EMBER_EXPORT_SERVICE void destroy_account(Service* service) {
	std::unique_ptr<Service>{service};
}

} // extern "C"

} // account, ember