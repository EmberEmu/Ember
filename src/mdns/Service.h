/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/LoggerFwd.h>
#include <shared/utility/cstring_view.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <chrono>
#include <exception>
#include <semaphore>

namespace ember::dns {

static constexpr cstring_view APP_NAME { "MDNS-SD" };

class Service {
	std::exception_ptr eptr;
	std::binary_semaphore stop_flag { 0 };

	log::Logger& logger;
	std::chrono::steady_clock::time_point start_time;

	void launch(const boost::program_options::variables_map& args, boost::asio::io_context& service);

public:
	static boost::program_options::options_description options();

	explicit Service(log::Logger& logger)
		: logger(logger),
		  start_time(std::chrono::steady_clock::now()) {}

	~Service() {
		stop();
	}

	int run(const boost::program_options::variables_map& args);
	void stop();
};

} // dns, ember