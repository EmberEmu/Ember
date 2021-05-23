/*
 * Copyright (c) 2015 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Core_generated.h"
#include <spark/Link.h>
#include <spark/EventHandler.h>
#include <spark/Helpers.h>
#include <logger/Logging.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <forward_list>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <cstdint>

namespace ember::spark::inline v1 {

class Service;

class HeartbeatService : public EventHandler {
	const std::chrono::seconds PING_FREQUENCY { 20 };
	const std::chrono::milliseconds LATENCY_WARN_THRESHOLD { 1000 };

	const Service* service_;
	std::forward_list<Link> peers_;
	std::mutex lock_;
	boost::asio::steady_timer timer_;
	std::unordered_map<messaging::core::Opcode, LocalDispatcher> handlers_;

	log::Logger* logger_;
	void set_timer();
	void send_ping(const Link& link, std::uint64_t time);
	void send_pong(const Link& link, std::uint64_t time);
	void trigger_pings(const boost::system::error_code& ec);
	void handle_ping(const Link& link, const Message& message);
	void handle_pong(const Link& link, const Message& message);

public:
	HeartbeatService(boost::asio::io_context& io_context, const Service* service,
	                 log::Logger* logger);

	void on_message(const Link& link, const Message& message) override;
	void on_link_up(const Link& link) override;
	void on_link_down(const Link& link) override;

	void shutdown();
};

} // spark, ember