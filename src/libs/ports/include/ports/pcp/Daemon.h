/*
 * Copyright (c) 2024 Ember
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
#include <functional>
#include <queue>
#include <vector>
#include <cstdint>

namespace ember::ports {

using namespace std::literals;

class Daemon {
	static constexpr auto TIMER_INTERVAL = 15s;
	static constexpr auto RENEW_WHEN_BELOW = 120s;

	struct Mapping {
		MapRequest request;
		std::chrono::steady_clock::time_point expiry;
	};

	Client& client_;
	boost::asio::steady_timer timer_;
	boost::asio::io_context::strand strand_;
	std::vector<Mapping> mappings_;
	std::queue<Mapping> queue_;
	std::uint32_t gateway_epoch_{};
	bool epoch_acquired_ = false;
	std::chrono::steady_clock::time_point daemon_epoch_;

	void process_queue();
	void start_renew_timer();
	void update_mapping(const Result& result);
	void erase_mapping(const Result& result);
	void renew_mappings();
	void renew_mapping(const Mapping& mapping);
	void check_epoch(std::uint32_t epoch);

public:
	Daemon(Client& client, boost::asio::io_context& ctx);

	void add_mapping(MapRequest request, RequestHandler&& handler);
	void delete_mapping(std::uint16_t internal_port, Protocol protocol, RequestHandler&& handler);
};

} // natpmp, ports, ember
