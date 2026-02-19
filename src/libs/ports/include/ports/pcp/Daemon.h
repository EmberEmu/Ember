/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/pcp/Client.h>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <deque>
#include <functional>
#include <vector>
#include <cstdint>

namespace ember::ports {

using namespace std::literals;

class Daemon final {
public:
	enum class Event {
		added_mapping,
		renewed_mapping,
		deleted_mapping,
		failed_to_renew,
		failed_mapping,
		mapping_expired
	};

	using EventHandler = std::function<void(Event, const MapRequest&)>;

private:
	static constexpr auto timer_interval   = 30s;
	static constexpr auto renew_when_below = 120s;
	static constexpr auto renew_leeway     = 2s;
	static constexpr auto max_consecutive_failures = 5;
	
	struct Mapping {
		MapRequest request;
		std::chrono::steady_clock::time_point expiry;
		bool strict;
		RequestHandler handler;
	};

	enum class State {
		idle,
		timer_wait,
		queue
	} state_ = State::idle;

	Client& client_;
	boost::asio::steady_timer timer_;
	boost::asio::io_context::strand strand_;
	std::vector<Mapping> mappings_;
	std::deque<Mapping> queue_;
	std::uint32_t gateway_epoch_{};
	bool epoch_acquired_ = false;
	std::chrono::steady_clock::time_point last_received_time_;
	EventHandler daemon_callback_{};
	int consecutive_failures_;

	void process_queue();
	void begin_timer();
	void start_renew_timer(std::chrono::seconds time = timer_interval);
	void update_mapping(const Result& result, const MapRequest& request);
	void erase_mapping(const Result& result);
	void renew_mappings();
	void renew_mapping(const Mapping& mapping);
	void check_epoch(std::uint32_t epoch);
	void invoke_daemon_callback(Event event, const MapRequest& request);

public:
	Daemon(Client& client, boost::asio::io_context& ctx, EventHandler&& handler = {});

	void add_mapping(MapRequest request, bool strict, RequestHandler&& handler);
	void delete_mapping(std::uint16_t internal_port, Protocol protocol, RequestHandler&& callback);
};

} // ports, ember
