/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/metrics/MetricsPoll.h>

namespace ember { 

MetricsPoll::MetricsPoll(boost::asio::io_context& service, Metrics& metrics)
                         : timer_(service), metrics_(metrics) {
	timer_.expires_from_now(FREQUENCY);

	timer_.async_wait([this](const boost::system::error_code& ec) {
		timeout(ec);
	});
}

void MetricsPoll::add_source(MetricsCB callback, std::chrono::seconds frequency) {
	std::lock_guard<std::mutex> guard(lock_);
	callbacks_.emplace_back(MetricMeta{ 0s, frequency, callback });
}

void MetricsPoll::shutdown() {
	timer_.cancel();
}

void MetricsPoll::timeout(const boost::system::error_code& ec) {
	if(ec) { // timer was cancelled
		return;
	}

	for(auto& cb : callbacks_) {
		cb.timer -=  FREQUENCY;

		// [under/over]flow check
		if(cb.timer > FREQUENCY || cb.timer < 0s) {
			cb.timer = 0s;
		}

		// time has elapsed, poll and reset timer
		if(cb.timer == 0s) {
			cb.callback(metrics_);
			cb.timer = cb.frequency;
		}
	}

	timer_.expires_from_now(FREQUENCY);
}

} // ember