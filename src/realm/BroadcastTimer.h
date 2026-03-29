/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "EventDispatcher.h"
#include <thread/ServicePool.h>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <vector>
#include <cstdlib>

namespace ember::realm {

using namespace std::chrono_literals;

/*
 * Implements a coarse-grained timer that fires a timer event notification to all connected
 * clients at the specified frequency.
 * 
 * This works by maintaining a timer per service thread rather than a single shared timer as it
 * allows for timers to fire without involving any cross-thread event posting. The number of
 * clients doesn't matter.
 * 
 * Overall, timers are quite expensive and this mitigates that by removing the need for
 * per client timers for events that don't need precise timing.
 */
class BroadcastTimer {
	constexpr static unsigned int max_offset { 20 };

	const std::chrono::milliseconds frequency_;
	const EventDispatcher& dispatcher_;
	const thread::ServicePool& pool_;
	std::vector<boost::asio::steady_timer> timers_;
	bool running_;

	void set_timer(boost::asio::steady_timer& timer, const std::chrono::seconds& offset = 0s) {
		timer.expires_after(frequency_ + offset);
		timer.async_wait([&](const auto& ec) {
			if(ec) {
				return;
			}

			dispatcher_.broadcast_event_thread({ EventType::interval_timer_fire });
			set_timer(timer);
		});
	}

public:
	BroadcastTimer(const thread::ServicePool& pool,
	               const EventDispatcher& dispatcher,
	               std::chrono::seconds frequency)
		: frequency_(frequency)
		, dispatcher_(dispatcher)
		, pool_(pool)
		, running_(false) {
		timers_.reserve(pool_.size());

		for(auto& ioc : pool_) {
			timers_.emplace_back(*ioc);
		}
	}

	~BroadcastTimer() {
		stop();
	}

	void start() {
		if(running_) {
			return;
		}

		// the initial firing will be slightly offset in case any of the events
		// do anything that could cause contention - can't guarantee separation
		// will be maintained over time but them's the breaks
		for(auto& timer : timers_) {
			set_timer(timer, std::chrono::seconds(std::rand() % max_offset));
		}

		running_ = true;
	}

	void stop() {
		timers_.clear();
		running_ = false;
	}
};

} // realm, ember