/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Config.h"
#include "ServiceContext.h"
#include <logger/LoggerFwd.h>
#include <commands/Registry.h>
#include <logger/LoggerFwd.h>
#include <service/Service.h>
#include <shared/utility/cstring_view.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <exception>
#include <chrono>
#include <memory>
#include <semaphore>

namespace ember::thread {
	class ServicePool;
};

namespace ember::realm {

static inline constexpr cstring_view app_name { "Realm Gateway" };

class EMBER_EXPORT_SERVICE Service final : public IService {
	log::Logger& logger;
	commands::Registry& registry;
	std::chrono::steady_clock::time_point start_time;
	ServiceContext context;
	std::binary_semaphore stop_flag;
	std::atomic_bool stopped;

	void register_commands();
	void update_config(const Config& config, bool post_only = false);
	boost::program_options::variables_map reload_args(const std::string& filename);
	Config generate_config(const boost::program_options::variables_map& args);
	void initialise(const boost::program_options::variables_map& args);

public:
	static boost::program_options::options_description options();

	Service(log::Logger& logger, commands::Registry& registry);
	~Service();

	int run(const boost::program_options::variables_map& args);
	void stop();

	static auto create(log::Logger& logger, commands::Registry& registry) {
		return std::make_unique<Service>(logger, registry);
	}
};

extern "C" EMBER_EXPORT_SERVICE Service* create_realm(log::Logger& logger, commands::Registry& registry);
extern "C" EMBER_EXPORT_SERVICE void destroy_realm(Service* service);

} // realm, ember