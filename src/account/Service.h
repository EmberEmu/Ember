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
#include <commands/PrefixedRegistry.h>
#include <shared/utility/cstring_view.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <chrono>

namespace ember::account {

constexpr cstring_view APP_NAME { "Account Daemon" };

class Service {
	log::Logger& logger;
	commands::PrefixedRegistry& cmd_register;
	std::chrono::steady_clock::time_point start_time;
	boost::asio::io_context service;
	ServiceContext context;

	void initialise(const boost::program_options::variables_map& args, boost::asio::io_context& service);
	void shutdown();

public:
	static boost::program_options::options_description options();

	Service(log::Logger& logger, commands::PrefixedRegistry& cmd_register);

	~Service() {
		stop();
	}

	int run(const boost::program_options::variables_map& args);
	void stop();
};

} // account, ember