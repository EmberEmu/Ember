/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include "FilterTypes.h"
#include "CharacterHandler.h"
#include "CharacterService.h"
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

/*
 * Starts Asio worker threads, blocking until the launch thread exits
 * upon error or signal handling.
 * 
 * io_context is only stopped after the thread joins to ensure that all
 * services can cleanly shut down upon destruction without requiring
 * explicit shutdown() calls in a signal handler.
 */
int Service::run(const opts::variables_map& args) try {
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

void Service::stop() {
	stop_flag.release();
}

void Service::launch(const opts::variables_map& args, boost::asio::io_context& service) try {
#ifdef DEBUG_NO_THREADS
	LOG_WARN_SYNC(logger, "Compiled with DEBUG_NO_THREADS!");
#endif

	LOG_INFO_SYNC(logger, "Loading DBC data...");
	dbc::DiskLoader loader(args["dbc.path"].as<std::string>(), [&](auto message) {
		LOG_DEBUG(logger) << message << LOG_SYNC;
	});

	auto dbc_store = loader.load(
		"ChrClasses", "ChrRaces", "CharBaseInfo", "NamesProfanity", "NamesReserved", "CharSections",
		"CharacterFacialHairStyles", "CharStartBase", "CharStartSpells", "CharStartSkills",
		"CharStartZones", "CharStartOutfit", "AreaTable", "FactionTemplate", "FactionGroup",
		"SpamMessages", "CharStartOutfit", "StartItemQuantities"
	);

	LOG_INFO_SYNC(logger, "Resolving DBC references...");
	dbc::link(dbc_store);

	LOG_INFO_SYNC(logger, "Compiling DBC regular expressions...");
	std::vector<utility::pcre::Result> profanity, reserved, spam;

	for(auto& record : dbc_store.names_profanity | std::views::values) {
		profanity.emplace_back(utility::pcre::utf8_jit_compile(record.name));
	}

	for(auto& record : dbc_store.names_reserved | std::views::values) {
		reserved.emplace_back(utility::pcre::utf8_jit_compile(record.name));
	}

	for(auto& record : dbc_store.spam_messages | std::views::values) {
		spam.emplace_back(utility::pcre::utf8_jit_compile(record.text));
	}

	LOG_INFO_SYNC(logger, "Initialising database driver...");
	const auto&  db_config_path = args["database.config_path"].as<std::string>();
	auto driver(drivers::init_db_driver(db_config_path, "login"));

	LOG_INFO_SYNC(logger, "Initialising database connection pool...");
	const auto concurrency = thread::hardware_concurrency([&](auto msg) {
		LOG_ERROR_SYNC(logger, "{}", msg);
	});

	const auto min_conns = args["database.min_connections"].as<unsigned short>();
	auto max_conns = args["database.max_connections"].as<unsigned short>();

	if(!max_conns) {
		max_conns = concurrency;
	} else if(max_conns != concurrency) {
		LOG_WARN_SYNC(logger, "Max. database connection count may be non-optimal "
		                      "(use {} to match logical core count)", concurrency);
	}

	LOG_INFO_SYNC(logger, "Initialising database connection pool...");

	connection_pool::Pool<decltype(driver), connection_pool::CheckinClean, connection_pool::ExponentialGrowth> pool(
		driver, min_conns, max_conns, 30s
	);
	
	pool.logging_callback([&](auto severity, auto message) {
		pool_log_callback(severity, message, logger);
	});

	LOG_INFO_SYNC(logger, "Initialising DAOs...");
	auto character_dao = dal::character_dao(pool);

	std::locale temp;

	const Config config {
		.defer_zone_placement = args["defer_zone_placement"].as<bool>()
	};

	thread::ThreadPool thread_pool(concurrency);
	CharacterHandler handler(std::move(profanity), std::move(reserved), std::move(spam),
	                         dbc_store, character_dao, config, thread_pool, temp, logger);

	const auto&  s_address = args["spark.address"].as<std::string>();
	auto s_port = args["spark.port"].as<std::uint16_t>();

	LOG_INFO_SYNC(logger, "Starting RPC services...");
	spark::Server spark(service, "character", s_address, s_port, logger);
	CharacterService char_service(spark, handler, logger);
	
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

} // character, ember