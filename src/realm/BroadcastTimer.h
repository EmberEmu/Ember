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

namespace ember::realm {

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
	const std::chrono::milliseconds frequency_;
	const EventDispatcher& dispatcher_;
	const thread::ServicePool& pool_;
	std::vector<boost::asio::steady_timer> timers_;
	bool stopped_;

public:
	BroadcastTimer(const thread::ServicePool& pool,
	               const EventDispatcher& dispatcher,
	               std::chrono::seconds frequency)
		: frequency_(frequency)
		, dispatcher_(dispatcher)
		, pool_(pool)
		, stopped_(true) {
		timers_.reserve(pool_.size());

		for(auto& ioc : pool_) {
			timers_.emplace_back(*ioc);
		}
	}

	~BroadcastTimer() {
		stop();
	}

	void start() {
		for(auto& timer : timers_) {
			set_timer(timer);
		}

		stopped_ = false;
	}

	void stop() {
		timers_.clear();
		stopped_ = true;
	}

	void set_timer(boost::asio::steady_timer& timer) {
		timer.expires_after(frequency_);
		timer.async_wait([&](const auto& ec) {
			if(ec) {
				return;
			}

			dispatcher_.broadcast_event_thread({ EventType::interval_timer_fire });
			set_timer(timer);
		});
	}
};

} // realm, ember