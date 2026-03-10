/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "LoggingCallbacks.h"
#include <conpool/ConnectionPool.h>
#include <conpool/Policies.h>
#include <conpool/drivers/AutoSelect.h>
#include <logger/Logger.h>
#include <boost/program_options/variables_map.hpp>

namespace ember::character {

auto init_database(const boost::program_options::variables_map& args,
                   log::Logger& logger) -> connection_pool::Pool<drivers::DriverType> {
	using namespace connection_pool;

	const auto& db_config_path = args["database.config_path"].as<std::string>();
	auto driver(drivers::init_db_driver(db_config_path, "login"));
	auto min_conns = args["database.min_connections"].as<unsigned short>();
	auto max_conns = args["database.max_connections"].as<unsigned short>();

	LOG_INFO_SYNC(logger, "Initialising database connection pool...");

	auto pool = create<drivers::AutoSelect, CheckinClean, ExponentialGrowth>(
		std::move(driver), min_conns, max_conns, 30s
	);

	pool->logging_callback([&](auto severity, auto message) {
		pool_log_callback(severity, message, logger);
	});

	return pool;
}

} // character, ember