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
#include <shared/utility/cstring_view.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <exception>
#include <chrono>
#include <semaphore>

namespace ember::thread {
	class ServicePool;
};

namespace ember::realm {

static inline constexpr cstring_view app_name { "Realm Gateway" };

class Service {
	log::Logger& logger;
	commands::Registry& registry;
	std::chrono::steady_clock::time_point start_time;
	ServiceContext context;
	std::binary_semaphore stop_flag;

	void initialise(const boost::program_options::variables_map& args, thread::ServicePool& service_pool);

public:
	static boost::program_options::options_description options();

	Service(log::Logger& logger, commands::Registry& registry);
	~Service();

	int run(const boost::program_options::variables_map& args);
	void stop();
};

} // realm, ember