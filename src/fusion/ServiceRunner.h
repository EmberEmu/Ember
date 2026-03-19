/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Wrapped.h"
#include <commands/Registry.h>
#include <logger/Logger.h>
#include <service/Service.h>
#include <boost/program_options.hpp>
#include <string>
#include <thread>

namespace ember::fusion {

namespace opts = boost::program_options;

class ServiceRunner final {
#ifdef BUILD_SHARED
	using ServiceHandle = Wrapped;
#else
	using ServiceHandle = IService*;
#endif

	std::jthread worker_;
	ServiceHandle service_;
	opts::variables_map opts_;
	std::unique_ptr<log::Logger> service_logger_;
	log::Logger& logger_;
	bool running_;

public:
	ServiceRunner(ServiceHandle service, opts::variables_map args, log::Logger& logger);

	void store_logger(std::unique_ptr<log::Logger> logger);
	void run();
	void stop();
	bool is_stopped();
	ServiceHandle handle();
};

} // fusion, ember