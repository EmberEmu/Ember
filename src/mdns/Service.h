/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ServiceContext.h"
#include <commands/Commands.h>
#include <logger/LoggerFwd.h>
#include <service/Service.h>
#include <shared/utility/cstring_view.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <atomic>
#include <chrono>
#include <memory>

namespace ember::dns {

static constexpr cstring_view app_name { "MDNS-SD" };

class EMBER_EXPORT_SERVICE Service final : public IService {
	log::Logger& logger;
	commands::Command& registry;
	std::chrono::steady_clock::time_point start_time;
	boost::asio::io_context ioc;
	ServiceContext context;
	std::atomic_bool stopped;

	void initialise(const boost::program_options::variables_map& args);

public:
	static boost::program_options::options_description options();

	Service(log::Logger& logger, commands::Command& registry);
	~Service();

	int run(const boost::program_options::variables_map& args);
	void stop();
};

extern "C" EMBER_EXPORT_SERVICE Service* create_mdns(log::Logger& logger, commands::Command& registry);
extern "C" EMBER_EXPORT_SERVICE void destroy_mdns(Service* service);

} // dns, ember