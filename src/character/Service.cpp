/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include "ServiceContextImpl.h"
#include "DBCRequired.h"
#include "FilterTypes.h"
#include "CharacterHandler.h"
#include "CharacterService.h"
#include "InitHelpers.h"
#include "LoggingCallbacks.h"
#include <dbcreader/Reader.h>
#include <conpool/ConnectionPool.h>
#include <conpool/Policies.h>
#include <conpool/drivers/AutoSelect.h>
#include <shared/database/daos/CharacterDAO.h>
#include <shared/utility/PCREHelper.h>
#include <shared/utility/Utility.h>
#include <spark/Server.h>
#include <thread/ThreadPool.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <chrono>
#include <ranges>
#include <cstddef>
#include <cstdlib>
#include <cstdint>

namespace opts = boost::program_options;
using namespace std::chrono_literals;

namespace ember::character {

Service::Service(log::Logger& logger, commands::Registry& registry)
	: logger(logger)
	, registry(registry) {
}

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

	LOG_INFO_SYNC(logger, "Loading DBC data...");
	dbc::DiskLoader loader(
		args["dbc.path"].as<std::string>(), [&](auto message) {
			LOG_DEBUG(logger) << message << LOG_SYNC;
		}
	);

	auto dbcs = loader.load(dbcs_required);
	ctx->dbcs = std::make_unique<dbc::Storage>(std::move(dbcs));

	LOG_INFO_SYNC(logger, "Resolving DBC references...");
	dbc::link(*ctx->dbcs);

	// todo, should probably do this somewhere else
	LOG_INFO_SYNC(logger, "Compiling DBC regular expressions...");
	std::vector<utility::pcre::Result> profanity, reserved, spam;

	for(auto& record : ctx->dbcs->names_profanity | std::views::values) {
		profanity.emplace_back(utility::pcre::utf8_jit_compile(record.name));
	}

	for(auto& record : ctx->dbcs->names_reserved | std::views::values) {
		reserved.emplace_back(utility::pcre::utf8_jit_compile(record.name));
	}

	for(auto& record : ctx->dbcs->spam_messages | std::views::values) {
		spam.emplace_back(utility::pcre::utf8_jit_compile(record.text));
	}

	const auto concurrency = thread::hardware_concurrency([&](auto msg) {
		LOG_ERROR_SYNC(logger, msg);
	});

	const auto min_conns = args["database.min_connections"].as<unsigned short>();
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
	auto character_dao = dal::character_dao(ctx->conn_pool->get());
	ctx->character_dao = std::make_unique<decltype(character_dao)>(std::move(character_dao));

	const Config config {
		.defer_zone_placement = args["defer_zone_placement"].as<bool>()
	};

	LOG_INFO_SYNC(logger, "Starting thread pool with {} threads...", concurrency);
	ctx->thread_pool = std::make_unique<thread::ThreadPool>(concurrency);

	LOG_INFO_SYNC(logger, "Starting character handler...");
	ctx->character_handler = std::make_unique<CharacterHandler>(
		std::move(profanity), std::move(reserved), std::move(spam),
	    *ctx->dbcs, *ctx->character_dao, config, *ctx->thread_pool, logger
	);

	const auto&  s_address = args["spark.address"].as<std::string>();
	const auto s_port = args["spark.port"].as<std::uint16_t>();

	LOG_INFO_SYNC(logger, "Starting RPC services...");
	ctx->spark = std::make_unique<spark::Server>(
		service, app_name, s_address, s_port, logger
	);

	ctx->character_service = std::make_unique<CharacterService>(
		*ctx->spark, *ctx->character_handler, logger
	);
	
	// All done setting up
	boost::asio::dispatch(service, [&, time]() {
		LOG_INFO_SYNC(logger, "{} started successfully in {}", app_name,
			utility::time_elapsed_format(time));

		start_time = std::chrono::steady_clock::now();
	});
}

void Service::stop() {
	LOG_TRACE_SYNC(logger, "{} shutting down...", app_name);
	auto ctx = context.get();
	ctx->thread_pool->shutdown();
	ctx->conn_pool->get().close();
}

Service::~Service() {
	LOG_TRACE_SYNC(logger, "Service termination requested");
	stop();
}

opts::options_description Service::options() {
	opts::options_description opts;
	opts.add_options()
		("defer_zone_placement", opts::value<bool>()->required())
		("dbc.path", opts::value<std::string>()->required())
		("spark.address", opts::value<std::string>()->required())
		("spark.port", opts::value<std::uint16_t>()->required())
		("nsd.host", opts::value<std::string>()->required())
		("nsd.port", opts::value<std::uint16_t>()->required())
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
		("file_log.path", opts::value<std::string>()->default_value("character.log"))
		("file_log.timestamp_format", opts::value<std::string>())
		("file_log.mode", opts::value<std::string>()->required())
		("file_log.size_rotate", opts::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", opts::value<bool>()->required())
		("file_log.log_timestamp", opts::value<bool>()->required())
		("file_log.log_severity", opts::value<bool>()->required())
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

extern "C" {

EMBER_EXPORT_SERVICE Service* create_character(log::Logger& logger, commands::Registry& registry) {
	return Service::create(logger, registry).release();
}

EMBER_EXPORT_SERVICE void destroy_character(Service* service) {
	std::unique_ptr<Service>{service};
}

} // extern "C"

} // character, ember