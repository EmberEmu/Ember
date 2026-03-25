/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Watchdog.h"
#include <logger/Logger.h>
#include <thread/Utility.h>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <cassert>

using namespace std::chrono_literals;

namespace ember {

Watchdog::Watchdog(std::chrono::seconds max_idle, log::Logger& logger)
	: max_idle_(std::move(max_idle)),
	  logger_(logger),
	  timeout_(false),
	  delta_(0s),
	  prev_(std::chrono::steady_clock::now()),
	  worker_(std::jthread(std::bind_front(&Watchdog::run, this))) { 
	if(max_idle_ <= 0s) {
		throw std::invalid_argument("max_idle must be > 0");
	}

	thread::set_name(worker_, "Watchdog");
}

void Watchdog::run(const std::stop_token token) {
	LOG_DEBUG(logger_, "Watchdog active ({} frequency)", max_idle_);

	std::mutex mutex;
	auto cond_var = std::condition_variable_any();

	while(!token.stop_requested()) {
		cond_var.wait_for(mutex, token, max_idle_, [&] {
			return check_timeout();
		});

		if(timeout_) [[unlikely]] {
			terminate();
		} else {
			timeout_ = true;
		}
	}

	LOG_DEBUG(logger_, "Watchdog stopped");
}

bool Watchdog::check_timeout() {
	const auto curr = std::chrono::steady_clock::now();
	delta_ = curr - prev_;

	// guard against spurious wake up
	if(delta_ < max_idle_) {
		return false;
	}

	prev_ = curr;
	return true;
}

void Watchdog::terminate() const {
	SLOG_FATAL(logger_, "Watchdog triggered after {}, terminating...",
	               std::chrono::duration_cast<std::chrono::seconds>(delta_));
	std::abort();
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