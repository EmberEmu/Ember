/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ServiceRunner.h"
#include <shared/utility/LogConfig.h>
#include <fstream>

namespace ember::fusion {

ServiceRunner::ServiceRunner(ServiceHandle service, opts::variables_map options, log::Logger& logger)
	: service_(service)
	, opts_(std::move(options))
	, logger_(logger)
	, running_(false) {}

void ServiceRunner::run() {
	running_ = true;

	worker_ = std::jthread([&] {
#ifdef BUILD_SHARED_SERVICES
		service_.service->run(opts_);
#else
		service_->run(opts_);
#endif
	});
}

void ServiceRunner::store_logger(std::unique_ptr<log::Logger> logger) {
	service_logger_ = std::move(logger);
}

void ServiceRunner::stop() {
#ifdef BUILD_SHARED_SERVICES
	service_.service->stop();
#else
	service_->stop();
#endif
	
	if(worker_.joinable()) {
		worker_.join();
	}

	running_ = false;
}

bool ServiceRunner::is_stopped() {
	return !running_;
}

auto ServiceRunner::handle() -> ServiceHandle {
	return service_;
}

} // fusion, ember