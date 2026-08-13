/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Forwards.h"
#include "Events.h"
#include <logger/LoggerFwd.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <array>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <cstddef>

using namespace std::chrono_literals;

namespace ember::realm {

class ShutdownAnnouncer {
public:
	using ShutdownFn = std::function<void()>;

private:
	constexpr static std::array<std::chrono::duration<int>, 20> intervals {
		60min, 45min, 30min, 15min, 5min, 60s, 45s, 30s, 15s,
		10s, 9s, 8s, 7s, 6s, 5s, 4s, 3s, 2s, 1s, 0s
	};

	using iterator = decltype(intervals)::const_iterator;

	EventDispatcher& dispatcher_;
	boost::asio::steady_timer timer_;
	log::Logger& logger_;
	bool active_;
	ShutdownFn callback_;
	std::chrono::steady_clock::time_point expires_;

	void reset();
	void run_timer(std::chrono::seconds expiry);
	void broadcast(std::string message);
	std::optional<std::chrono::duration<int>> nearest_interval(std::chrono::duration<int> duration);

	static std::string time_remaining_fmt(std::chrono::seconds remaining);

public:
	ShutdownAnnouncer(boost::asio::io_context& ioc, ShutdownFn callback,
	                  EventDispatcher& dispatcher, log::Logger& logger);

	void set_at(std::chrono::steady_clock::time_point time, bool announce);
	void set_after(std::chrono::seconds expiry, bool announce);
	std::chrono::steady_clock::time_point expires() const;
	bool pending() const;
	void stop();
};

} // realm, ember
