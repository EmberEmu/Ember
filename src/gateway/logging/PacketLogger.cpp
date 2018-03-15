/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PacketLogger.h"
#include <chrono>

namespace sc = std::chrono;

namespace ember {

void PacketLogger::add_sink(std::unique_ptr<PacketSink> sink) {
	sinks_.emplace_back(sink);
}

void PacketLogger::reset() {
	sinks_.clear();
}

void PacketLogger::log(spark::Buffer& buffer, std::size_t length) {
	auto time = sc::time_point_cast<sc::milliseconds>(sc::system_clock::now());
	auto timestamp = time.time_since_epoch().count();

	for(auto& sink : sinks_) {
		sink->log(buffer, length, timestamp);
	}
}

} // ember

