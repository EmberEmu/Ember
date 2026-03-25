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
	, registry(registry)
	, stopped(false) {
}

int Service::run(const opts::variables_map& args) try {
	initialise(args);
	ioc.run();

	SLOG_INFO(logger, "{} shutting down...", app_name);
	return EXIT_SUCCESS;
} catch(const std::exception& e) {
	SLOG_FATAL(logger, e.what());
	return EXIT_FAILURE;
}

void Service::initialise(const opts::variables_map& args) {
	const auto time = std::chrono::steady_clock::now();
	auto ctx = context.get();

	const auto& iface = args["mdns.interface"].as<std::string>();
	const auto& group = args["mdns.group"].as<std::string>();
	const auto port = args["mdns.port"].as<std::uint16_t>();

	// start multicast DNS services
	SLOG_INFO(logger, "Starting multicaster...");
	auto socket = std::make_unique<dns::MulticastSocket>(ioc, iface, group, port, logger);

	SLOG_INFO(logger, "Starting DNS server handler...");
	ctx->server = std::make_unique<Server>(std::move(socket), logger);

	const auto& spark_iface = args["spark.address"].as<std::string>();
	const auto spark_port = args["spark.port"].as<std::uint16_t>();

	// start RPC services
	SLOG_INFO(logger, "Starting RPC services...");
	ctx->spark = std::make_unique<spark::Server>(ioc, app_name, spark_iface, spark_port, logger);
	ctx->nsd_service = std::make_unique<NSDService>(*ctx->spark, logger);

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

	boost::asio::post(ioc, [&] {
		auto ctx = context.get();
		ctx->server->shutdown();
	});
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
		("metrics.statsd_port", opts::value<std::uint16_t>()->required());
	return opts;
}

extern "C" {

EMBER_EXPORT_SERVICE Service* create_mdns(log::Logger& logger, commands::Registry& registry) {
	return new Service(logger, registry);
}

EMBER_EXPORT_SERVICE void destroy_mdns(Service* service) {
	delete service;
}

} // extern "C"

} // dns, ember