/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio.hpp>
#include <array>
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

// todo - workaround for GCC defect, awaiting fix (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60970)
struct GCCHashFix { template <typename T> std::size_t operator()(T t) const { return static_cast<std::size_t>(t); }};

class Monitor {
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

	typedef std::function<void(const Source, Severity, std::intmax_t)> LogCallback;

private:
	const std::chrono::seconds TIMER_FREQUENCY;
	boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
	boost::asio::ip::udp::socket socket_;
	boost::asio::ip::udp::endpoint endpoint_;
	boost::asio::signal_set signals_;
	boost::asio::strand strand_;

	std::vector<std::tuple<Source, Severity, LogCallback, std::chrono::seconds>> sources_;
	std::mutex source_lock_;
	std::unordered_map<Severity, unsigned int, GCCHashFix> counters_;
	std::array<char, 1> buffer_;

	void receive();
	void set_timer();
	void send_health_status();
	void timer_tick(const boost::system::error_code& ec);
	void execute_source(Source& source, Severity severity, const LogCallback& log,
	                    std::chrono::seconds& last_tick);
	std::string generate_message();
	void shutdown();

public:
	Monitor(boost::asio::io_service& service, const std::string& interface,
	        std::uint16_t port, std::chrono::seconds frequency = 5s);

	void add_source(Source source, Severity severity, LogCallback log_callback);
};

} // ember