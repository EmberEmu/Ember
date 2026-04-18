/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ShutdownAnnouncer.h"
#include "EventDispatcher.h"
#include <shared/utility/Utility.h>
#include <format>
#include <cassert>

namespace ember::realm {

ShutdownAnnouncer::ShutdownAnnouncer(boost::asio::io_context& ioc, ShutdownFn fn,
                                     EventDispatcher& dispatcher, log::Logger& logger)
	: dispatcher_(dispatcher)
	, timer_(ioc)
	, logger_(logger)
	, active_(false)
	, fn_(std::move(fn)) {
	assert(fn_);
}

void ShutdownAnnouncer::set_at(std::chrono::steady_clock::time_point time, bool announce) {
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
		time - std::chrono::steady_clock::now()
	);

	set_after(seconds, announce);
}

void ShutdownAnnouncer::set_after(std::chrono::seconds expiry, bool announce) {
	if(expiry < 0s) {
		return;
	}

	reset();
	auto message = std::format("Shutdown in {}", utility::time_duration_format(expiry));

	if(announce) {
		broadcast(std::move(message));
	} else {
		LOG_INFO(logger_, message);
	}

	active_ = true;
	expires_ = std::chrono::steady_clock::now() + expiry;

	run_timer(expiry);
}

auto ShutdownAnnouncer::nearest_interval(std::chrono::duration<int> duration) -> iterator {
	for(auto i = intervals.begin(); i < intervals.end(); ++i) {
		if(duration > *i) {
			return i;
		}
	}

	return intervals.end();
}

void ShutdownAnnouncer::run_timer(std::chrono::seconds expiry) {
	const auto it = nearest_interval(expiry);

	if(it == intervals.end()) {
		fn_();
		return;
	}

	// calculate how long we need to wait for 
	auto wait_for = expiry - *it;

	timer_.expires_after(wait_for);
	timer_.async_wait([&, it](auto ec) {
		if(ec) {
			return;
		}

		broadcast(std::format("Shutdown in {}", time_remaining_fmt(*it)));
		run_timer(*it);
	});
}

std::string ShutdownAnnouncer::time_remaining_fmt(std::chrono::seconds remaining) {
	if(remaining >= 3600s) {
		int hours = remaining / 3600s;
		return hours == 1? "1 hour" : std::to_string(hours) + " hours";
	}

	if(remaining >= 60s) {
		const auto minutes = remaining / 60s;
		return minutes == 1? "1 minute" : std::to_string(minutes) + " minutes";
	}

	return remaining == 1s? "1 second" : std::to_string(remaining.count()) + " seconds";
}

void ShutdownAnnouncer::broadcast(std::string message) {
	LOG_INFO(logger_, message);
	SystemMessage event(std::move(message), SystemMessage::Type::message);
	dispatcher_.broadcast(event);
}

void ShutdownAnnouncer::reset() {
	stop();
}

void ShutdownAnnouncer::stop() {
	if(active_) {
		broadcast("Shutdown cancelled");
	}

	timer_.cancel();
	active_ = false;
}

std::chrono::steady_clock::time_point ShutdownAnnouncer::expires() const {
	return expires_;
}

bool ShutdownAnnouncer::pending() const {
	return active_;
}

} // realm, ember