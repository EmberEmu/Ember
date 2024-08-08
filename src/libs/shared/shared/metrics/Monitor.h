/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/ip/udp.hpp>
#include <chrono>
#include <functional>
#include <string>
#include <mutex>
#include <unordered_map>
#include <tuple>
#include <vector>
#include <cstdint>

#undef ERROR // blame Windows' headers

namespace ember {

using namespace std::chrono_literals;

class Monitor final {
public:
	enum class Severity {
		DEBUG, INFO, WARN, ERROR, FATAL
	};

	struct Source {
		std::string key;
		std::function<std::intmax_t()> callback;
		std::chrono::seconds frequency;
		std::intmax_t threshold;
		std::function<bool(std::intmax_t, std::intmax_t)> comparator;
		std::string message;
		bool triggered;
	};

	using LogCallback = std::function<void(const Source, Severity, std::intmax_t)>;

private:
	const std::chrono::seconds TIMER_FREQUENCY;
	boost::asio::steady_timer timer_;
	boost::asio::ip::udp::socket socket_;
	boost::asio::ip::udp::endpoint endpoint_;
	boost::asio::signal_set signals_;
	boost::asio::io_context::strand strand_;

	std::vector<std::tuple<Source, Severity, LogCallback, std::chrono::seconds>> sources_;
	mutable std::mutex source_lock_;
	std::unordered_map<Severity, unsigned int> counters_;

	void receive();
	void set_timer();
	void send_health_status();
	void timer_tick(const boost::system::error_code& ec);
	void execute_source(Source& source, Severity severity, const LogCallback& log,
	                    std::chrono::seconds& last_tick);
	std::string generate_message() const;
	bool has_severity(Severity sev) const;
	void shutdown();

public:
	Monitor(boost::asio::io_context& service, const std::string& interface,
	        std::uint16_t port, std::chrono::seconds frequency = 5s);

	void add_source(Source source, Severity severity, LogCallback log_callback);
};

} // ember