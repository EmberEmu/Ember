/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ServiceContext.h"
#include <commands/Commands.h>
#include <service/Service.h>
#include <boost/program_options.hpp>
#include <string>
#include <thread>

namespace ember::fusion {

namespace opts = boost::program_options;

class ServiceRunner final {
	std::jthread worker_;
	ServiceContext context_;
	opts::variables_map opts_;
	bool running_;

public:
	ServiceRunner(ServiceContext context, opts::variables_map args);

	void run();
	void stop();
	bool is_stopped() const;
	const ServiceContext& context() const;
};

} // fusion, ember