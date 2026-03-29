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

Service::Service(log::Logger& logger, commands::Command& registry)
	: logger(logger)
	, registry(registry)
	, stopped(false) {}

int Service::run(const opts::variables_map& args) try {
	initialise(args);
	ioc.run();
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	SLOG_FATAL(logger, e.what());
	return EXIT_FAILURE;
}

void Service::initialise(const opts::variables_map& args) {
	const auto time = std::chrono::steady_clock::now();
	auto ctx = context.get();

	SLOG_INFO(logger, "Loading DBC data...");
	dbc::DiskLoader loader(
		args["dbc.path"].as<std::string>(), [&](auto message) {
			SLOG_DEBUG(logger, message);
		}
	);

	auto dbcs = loader.load(dbcs_required);
	ctx->dbcs = std::make_unique<dbc::Storage>(std::move(dbcs));

	SLOG_INFO(logger, "Resolving DBC references...");
	dbc::link(*ctx->dbcs);

	// todo, should probably do this somewhere else
	SLOG_INFO(logger, "Compiling DBC regular expressions...");
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
		SLOG_ERROR(logger, msg);
	});

	const auto min_conns = args["database.min_connections"].as<unsigned short>();
	auto max_conns = args["database.max_connections"].as<unsigned short>();

	if(!max_conns) {
		max_conns = concurrency;
	}

	SLOG_INFO(logger, "Max. database connections set to {} ({} cores)", max_conns, concurrency);

	SLOG_INFO(logger, "Initialising database connection pool...");
	ctx->conn_pool = std::make_unique<connection_pool::Pool<drivers::AutoSelect>>(
		init_database(args, logger)
	);

	SLOG_INFO(logger, "Initialising DAOs...");
	auto character_dao = dal::character_dao(ctx->conn_pool->get());
	ctx->character_dao = std::make_unique<decltype(character_dao)>(std::move(character_dao));

	const Config config {
		.defer_zone_placement = args["defer_zone_placement"].as<bool>(),
		.max_chars_slots_account = args["max_chars_slots_account"].as<unsigned int>(),
		.max_chars_slots_server = args["max_chars_slots_server"].as<unsigned int>()
	};

	SLOG_INFO(logger, "Starting thread pool with {} threads...", concurrency);
	ctx->thread_pool = std::make_unique<thread::ThreadPool>(concurrency);

	SLOG_INFO(logger, "Starting character handler...");
	ctx->character_handler = std::make_unique<CharacterHandler>(
		std::move(profanity), std::move(reserved), std::move(spam),
	    *ctx->dbcs, *ctx->character_dao, config, *ctx->thread_pool, logger
	);

	const auto&  s_address = args["spark.address"].as<std::string>();
	const auto s_port = args["spark.port"].as<std::uint16_t>();

	SLOG_INFO(logger, "Starting RPC services...");
	ctx->spark = std::make_unique<spark::Server>(
		ioc, app_name, s_address, s_port, logger
	);

	ctx->character_service = std::make_unique<CharacterService>(
		*ctx->spark, *ctx->character_handler, logger
	);
	
	// All done setting up
	boost::asio::dispatch(ioc, [&, time]() {
		SLOG_INFO(logger, "{} started successfully in {}", app_name,
			utility::time_elapsed_format(time));

		start_time = std::chrono::steady_clock::now();
	});
}

void Service::stop() {
	bool expected = false;

	if(!stopped.compare_exchange_strong(expected, true)) {
		return;
	}

	SLOG_TRACE(logger, "Service termination requested");
	auto ctx = context.get();

	boost::asio::post(ioc, [&] {
		auto ctx = context.get();
		ctx->thread_pool->shutdown();
		ctx->conn_pool->get().close();
	});
}

Service::~Service() {
	SLOG_TRACE(logger, "Service termination requested");
	stop();
}

opts::options_description Service::options() {
	opts::options_description opts;
	opts.add_options()
		("defer_zone_placement", opts::value<bool>()->required())
		("max_chars_slots_account", opts::value<unsigned int>()->required())
		("max_chars_slots_server", opts::value<unsigned int>()->required())
		("dbc.path", opts::value<std::string>()->required())
		("spark.address", opts::value<std::string>()->required())
		("spark.port", opts::value<std::uint16_t>()->required())
		("nsd.host", opts::value<std::string>()->required())
		("nsd.port", opts::value<std::uint16_t>()->required())
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

EMBER_EXPORT_SERVICE Service* create_character(log::Logger& logger, commands::Command& registry) {
	return new Service(logger, registry);
}

EMBER_EXPORT_SERVICE void destroy_character(Service* service) {
	delete service;
}

} // extern "C"

} // character, ember