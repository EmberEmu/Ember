/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include "ServiceContextImpl.h"
#include "Server.h"
#include "MulticastSocket.h"
#include "NSDService.h"
#include <logger/Logger.h>
#include <spark/Server.h>
#include <thread/Utility.h>
#include <shared/utility/Utility.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <memory>
#include <utility>
#include <cstddef>
#include <cstdlib>

namespace opts = boost::program_options;

namespace ember::dns {

Service::Service(log::Logger& logger, commands::Registry& registry)
	: logger(logger)
	, registry(registry) {
}

int Service::run(const opts::variables_map& args) try {
	initialise(args);
	service.run();

	LOG_INFO_SYNC(logger, "{} shutting down...", app_name);
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	LOG_FATAL_SYNC(logger, "{}", e.what());
	return EXIT_FAILURE;
}

void Service::initialise(const opts::variables_map& args) {
	const auto time = std::chrono::steady_clock::now();
	auto ctx = context.get();

	const auto& iface = args["mdns.interface"].as<std::string>();
	const auto& group = args["mdns.group"].as<std::string>();
	const auto port = args["mdns.port"].as<std::uint16_t>();

	// start multicast DNS services
	LOG_INFO_SYNC(logger, "Starting multicaster...");
	auto socket = std::make_unique<dns::MulticastSocket>(service, iface, group, port, logger);

	LOG_INFO_SYNC(logger, "Starting DNS server handler...");
	ctx->server = std::make_unique<Server>(std::move(socket), logger);

	const auto& spark_iface = args["spark.address"].as<std::string>();
	const auto spark_port = args["spark.port"].as<std::uint16_t>();

	// start RPC services
	LOG_INFO_SYNC(logger, "Starting RPC services...");
	ctx->spark = std::make_unique<spark::Server>(service, app_name, spark_iface, spark_port, logger);
	ctx->nsd_service = std::make_unique<NSDService>(*ctx->spark, logger);

	// All done setting up
	boost::asio::dispatch(service, [&]() {
		LOG_INFO_SYNC(logger, "{} started successfully in {}", app_name,
			utility::start_time_format(time));

		start_time = std::chrono::steady_clock::now();
	});
}

void Service::stop() {
	LOG_TRACE_SYNC(logger, "{} shutting down...", app_name);
	auto ctx = context.get();
	ctx->server->shutdown();
}

Service::~Service() {
	stop();
}

opts::options_description Service::options() {
	opts::options_description opts;
	opts.add_options()
		("mdns.interface", opts::value<std::string>()->required())
		("mdns.group", opts::value<std::string>()->required())
		("mdns.port", opts::value<std::uint16_t>()->default_value(5353))
		("spark.address", opts::value<std::string>()->required())
		("spark.port", opts::value<std::uint16_t>()->required())
		("metrics.enabled", opts::value<bool>()->required())
		("metrics.statsd_host", opts::value<std::string>()->required())
		("metrics.statsd_port", opts::value<std::uint16_t>()->required())
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
		("file_log.path", opts::value<std::string>()->default_value("mdns.log"))
		("file_log.timestamp_format", opts::value<std::string>())
		("file_log.mode", opts::value<std::string>()->required())
		("file_log.size_rotate", opts::value<std::uint32_t>()->required())
		("file_log.midnight_rotate", opts::value<bool>()->required())
		("file_log.log_timestamp", opts::value<bool>()->required())
		("file_log.log_severity", opts::value<bool>()->required());
	return opts;
}

extern "C" {

EMBER_EXPORT_SERVICE Service* create_mdns(log::Logger& logger, commands::Registry& registry) {
	return Service::create(logger, registry).release();
}

EMBER_EXPORT_SERVICE void destroy_mdns(Service* service) {
	std::unique_ptr<Service>{service};
}

} // extern "C"

} // dns, ember