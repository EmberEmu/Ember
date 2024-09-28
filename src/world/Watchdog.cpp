/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Watchdog.h"
#include <condition_variable>
#include <functional>
#include <mutex>

namespace ember {

Watchdog::Watchdog(log::Logger& logger, std::chrono::seconds max_idle)
	: logger_(logger),
	  max_idle_(max_idle),
	  timeout_(false),
	  prev_(std::chrono::steady_clock::now()),
	  worker_(std::jthread(std::bind_front(&Watchdog::monitor, this))) { }

void Watchdog::monitor(std::stop_token token) {
	LOG_DEBUG_ASYNC(logger_, "Watchdog active ({} frequency)", max_idle_);

	std::mutex cv_mtx_;

	while(!token.stop_requested()) {
		check();
		std::condition_variable_any().wait_for(cv_mtx_, token, max_idle_, [] { return false; });
	}

	LOG_DEBUG_ASYNC(logger_, "Watchdog stopped");
}

void Watchdog::check() {
	const auto curr = std::chrono::steady_clock::now();
	const auto delta = curr - prev_;

	// guard against spurious wake up
	if(delta < max_idle_) {
		return;
	}

	if(timeout_) {
		const auto delta_s = std::chrono::duration_cast<std::chrono::seconds>(delta);
		LOG_FATAL_SYNC(logger_, "Watchdog triggered after {}, terminating...", delta_s);
		std::abort();
	}

	prev_ = curr;
	timeout_ = true;
}

void Watchdog::notify() {
	timeout_ = false;
}

void Watchdog::stop() {
	worker_.request_stop();
}

Watchdog::~Watchdog() {
	stop();
}

} // ember