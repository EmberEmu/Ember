/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/metrics/Metrics.h>
#include <boost/asio.hpp>
#include <chrono>
#include <string>
#include <cstdint>

namespace ember {

class MetricsImpl final : public Metrics {
	boost::asio::signal_set signals_;
	boost::asio::ip::udp::socket socket_;
	boost::asio::ip::udp::endpoint endpoint_;

	void send(std::string message);
	void shutdown();

public:
	MetricsImpl(boost::asio::io_service& service, const std::string& host, std::uint16_t port);

	void increment(const char* key, std::intmax_t value = 1) override;
	void timing(const char* key, std::chrono::milliseconds value) override;
	void gauge(const char* key, std::size_t value, bool adjust = false) override;
	void set(const char* key, std::intmax_t value) override;
};

} // ember