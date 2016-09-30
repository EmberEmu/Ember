/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/metrics/Metrics.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_waitable_timer.hpp>
#include <chrono>
#include <functional>
#include <mutex>

namespace ember {

using namespace std::chrono_literals;

class MetricsPoll {
	typedef std::function<void(Metrics& metrics)> MetricsCB;
	const std::chrono::seconds FREQUENCY = 1s;

	struct MetricMeta {
		std::chrono::seconds timer;
		std::chrono::seconds frequency;
		MetricsCB callback;
	};

	std::mutex lock_;
	boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
	std::vector<MetricMeta> callbacks_;
	Metrics& metrics_;

	void timeout(const boost::system::error_code& ec);

public:
	MetricsPoll(boost::asio::io_service& service, Metrics& metrics);

	void add_source(MetricsCB callback, std::chrono::seconds frequency);
	void shutdown();
};

} // ember