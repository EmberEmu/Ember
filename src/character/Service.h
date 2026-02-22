/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/LoggerFwd.h>
#include <commands/PrefixedRegistry.h>
#include <shared/utility/cstring_view.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <chrono>
#include <exception>
#include <semaphore>

namespace ember::character {

constexpr cstring_view APP_NAME { "Character Daemon" };

class Service {
	std::exception_ptr eptr;
	std::binary_semaphore stop_flag { 0 };

	log::Logger& logger;
	commands::PrefixedRegistry& cmd_register;
	std::chrono::steady_clock::time_point start_time;

	void launch(const boost::program_options::variables_map& args, boost::asio::io_context& service);
public:
	static boost::program_options::options_description options();

	explicit Service(log::Logger& logger, commands::PrefixedRegistry& cmd_register)
		: logger(logger),
		  cmd_register(cmd_register),
		  start_time(std::chrono::steady_clock::now()) {}

	~Service() {
		stop();
	}

	int run(const boost::program_options::variables_map& args);
	void stop();
};

} // character, ember