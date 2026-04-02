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

ServiceRunner::ServiceRunner(ServiceContext context, opts::variables_map options)
	: context_(std::move(context))
	, opts_(std::move(options))
	, running_(false) {}

ServiceRunner::~ServiceRunner() {
	stop();
}

void ServiceRunner::run() {
	running_ = true;

	// the copies are required to ensure that this object is safe to move
	// after run has already been called
	worker_ = std::jthread([service = context_.service, opts = opts_] {
		service->run(opts);
	});
}

void ServiceRunner::stop() {
	if(!running_) {
		return;
	}

	context_.service->stop();
	
	if(worker_.joinable()) {
		worker_.join();
	}

	running_ = false;
}

bool ServiceRunner::is_stopped() const {
	return !running_;
}
const ServiceContext& ServiceRunner::context() const{
	return context_;
}

} // fusion, ember