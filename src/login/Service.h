/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ServiceContext.h"
#include <logger/LoggerFwd.h>
#include <commands/Registry.h>
#include <shared/metrics/Metrics.h>
#include <thread/Utility.h>
#include <shared/utility/cstring_view.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <atomic>
#include <chrono>
#include <exception>
#include <memory>

namespace ember::login {

constexpr cstring_view app_name { "Login Daemon" };

class Service {
	log::Logger& logger;
	commands::Registry& registry;
	std::chrono::steady_clock::time_point start_time;
	boost::asio::io_context io_context;
	boost::asio::io_context::strand serialise;
	std::atomic_bool stopped;
	ServiceContext context;

	std::unique_ptr<Metrics> start_metrics(
		boost::asio::io_context& service,
		const boost::program_options::variables_map& args
	);

	void initialise(const boost::program_options::variables_map& args);

public:
	static boost::program_options::options_description options();

	explicit Service(log::Logger& logger, commands::Registry& registry)
		: logger(logger)
		, registry(registry)
		, start_time(std::chrono::steady_clock::now())
		, io_context(thread::hardware_concurrency())
		, serialise(io_context) {}

	~Service();

	int run(const boost::program_options::variables_map& args);
	void stop();
};

} // login, ember