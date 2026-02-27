/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
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

	const auto& iface = args["mdns.interface"].as<std::string>();
	const auto& group = args["mdns.group"].as<std::string>();
	const auto port = args["mdns.port"].as<std::uint16_t>();

	// start multicast DNS services
	auto socket = std::make_unique<dns::MulticastSocket>(service, iface, group, port, logger);
	dns::Server server(std::move(socket), logger);

	const auto& spark_iface = args["spark.address"].as<std::string>();
	const auto spark_port = args["spark.port"].as<std::uint16_t>();

	// start RPC services
	spark::Server spark(service, APP_NAME, spark_iface, spark_port, logger);
	NSDService nsd(spark, logger);

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

} // dns, ember