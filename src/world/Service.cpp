/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include "ServiceContextImpl.h"
#include "MapRunner.h"
#include "DBCRequired.h"
#include "utilities/Utility.h"
#include "WorldRPCServer.h"
#include <logger/Logger.h>
#include <dbcreader/Reader.h>
#include <shared/utility/Utility.h>
#include <spark/Server.h>
#include <boost/program_options.hpp>
#include <thread>
#include <vector>
#include <cstdint>
#include <cstdlib>

namespace opts = boost::program_options;

namespace ember::world {

Service::Service(log::Logger& logger, commands::Registry& registry)
	: logger(logger)
	, registry(registry)
	, stop_flag(false) {}

int Service::run(const boost::program_options::variables_map& args) {
	const auto time = std::chrono::steady_clock::now();
	auto ctx = context.get();

	LOG_INFO_SYNC(logger, "Loading DBC data...");

	dbc::DiskLoader loader(args["dbc.path"].as<std::string>(), [&](auto message) {
		LOG_DEBUG(logger) << message << LOG_SYNC;
	});

	ctx->dbcs = std::make_unique<dbc::Storage>(std::move(loader.load(dbcs_required)));

	LOG_INFO_SYNC(logger, "Resolving DBC references...");
	dbc::link(*ctx->dbcs);

	const auto tip = random_tip(ctx->dbcs->game_tips);

	if(!tip.empty()) {
		LOG_INFO_SYNC(logger, "Tip: {}", tip);
	}

	const auto& maps = args["world.map_id"].as<std::vector<std::int32_t>>();

	if(!validate_maps(maps, ctx->dbcs->map, logger)) {
		return EXIT_FAILURE;
	}

	LOG_INFO_SYNC(logger, "Serving as world server for maps:");
	print_maps(maps, ctx->dbcs->map, logger);

	// temporary bits
	const auto interface = args["spark.address"].as<std::string>();
	const auto port = args["spark.port"].as<std::uint16_t>();

	ctx->spark = std::make_unique<spark::Server>(service, app_name, interface, port, logger);
	ctx->world_rpc_service = std::make_unique<WorldRPCServer>(*ctx->spark, logger);

	std::jthread thread(&boost::asio::io_context::run, &service);
	// end of temporary bits

	// All done setting up
	LOG_INFO_SYNC(logger, "{} started successfully in {}", app_name, utility::time_elapsed_format(time));
	start_time = time;

	map::run(logger, stop_flag);

	// temp bits again
	ctx->spark->shutdown();
	LOG_INFO_SYNC(logger, "{} terminated", app_name);
	return EXIT_SUCCESS;
}

void Service::stop() {
	stop_flag = true;
}

Service::~Service() {
	stop();
}

opts::options_description Service::options() {
	opts::options_description opts;
	opts.add_options()
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
		("file_log.path", opts::value<std::string>()->default_value("world.log"))
		("file_log.timestamp_format", opts::value<std::string>())
		("file_log.mode", opts::value<std::string>()->required())
		("file_log.size_rotate", opts::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", opts::bool_switch()->required())
		("file_log.log_timestamp", opts::value<bool>()->required())
		("file_log.log_severity", opts::value<bool>()->required())
		("database.min_connections", opts::value<unsigned short>()->required())
		("database.max_connections", opts::value<unsigned short>()->required())
		("database.config_path", opts::value<std::string>()->required())
		("network.interface", opts::value<std::string>()->required())
		("network.port", opts::value<std::uint16_t>()->required())
		("network.tcp_no_delay", opts::value<bool>()->required())
		("dbc.path", opts::value<std::string>()->required())
		("spark.address", opts::value<std::string>()->required())
		("spark.port", opts::value<std::uint16_t>()->required())
		("nsd.host", opts::value<std::string>()->required())
		("nsd.port", opts::value<std::uint16_t>()->required())
		("world.id", opts::value<std::uint32_t>()->required())
		("world.map_id", opts::value<std::vector<std::int32_t>>()->required());
	return opts;
}

extern "C" {

EMBER_EXPORT_SERVICE Service* create_world(log::Logger& logger, commands::Registry& registry) {
	return Service::create(logger, registry).release();
}

EMBER_EXPORT_SERVICE void destroy_world(Service* service) {
	std::unique_ptr<Service>{service};
}

} // extern "C"

} // world, ember