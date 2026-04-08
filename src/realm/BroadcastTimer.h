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
#include <boost/asio/awaitable.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/post.hpp>
#include <chrono>
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
	template<typename T>
	using await = boost::asio::awaitable<T>;

	constexpr static auto max_offset { 10u };
	constexpr static Event event { EventType::interval_timer_fire };

	const std::chrono::milliseconds frequency_;
	const EventDispatcher& dispatcher_;
	const thread::ServicePool& pool_;
	std::atomic_bool running_;

	await<void> await_timer(boost::asio::steady_timer& timer, const std::chrono::seconds offset) {
		timer.expires_after(frequency_ + offset);
		const auto& [ec] = co_await timer.async_wait(boost::asio::as_tuple);

		if(ec) {
			co_return;
		}

		dispatcher_.broadcast_self(event);
	}

	// the initial firing will be slightly offset in case any of the events
	// do anything that could cause contention - can't guarantee separation
	// will be maintained over time but them's the breaks
	await<void> run(std::shared_ptr<boost::asio::steady_timer> timer) {
		auto offset = std::chrono::seconds(std::rand() % max_offset);

		while(running_) {
			co_await await_timer(*timer, offset);
			offset = 0s; // only set it once
		}
	}

	struct Worker {
		boost::asio::io_context& ioc;
		std::shared_ptr<boost::asio::steady_timer> timer;
	};

	std::vector<Worker> workers_;

public:
	BroadcastTimer(const thread::ServicePool& pool,
	               const EventDispatcher& dispatcher,
	               std::chrono::seconds frequency)
		: frequency_(frequency)
		, dispatcher_(dispatcher)
		, pool_(pool)
		, running_(false) {}

	~BroadcastTimer() {
		stop();
	}

	void start() {
		bool expected = false;

		if(!running_.compare_exchange_strong(expected, true)) {
			return;
		}

		for(auto& ioc : pool_.services()) {
			auto timer = std::make_shared<boost::asio::steady_timer>(ioc);
			boost::asio::co_spawn(ioc, run(timer), boost::asio::detached);
			workers_.emplace_back(ioc, std::move(timer));
		}
	}

	void stop() {
		running_ = false;

		for(auto& worker : workers_) {
			auto& timer = worker.timer;

			boost::asio::post(worker.ioc, [timer] {
				timer->cancel();
			});
		}
	}
};

} // realm, ember