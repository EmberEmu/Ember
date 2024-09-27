/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "MapRunner.h"
#include <logger/Logger.h>
#include <shared/util/Timing.h>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace ember::world {

const auto UPDATE_FREQUENCY = 60.0;
const std::chrono::duration<double> UPDATE_DELTA { 1000ms / UPDATE_FREQUENCY };
const auto TIME_PERIOD = 1ms;

// placeholder
void update(std::chrono::milliseconds delta) {
	
}

void run(log::Logger& log) {
	LOG_TRACE(log) << log_func << LOG_ASYNC;

	auto timer_guard = util::set_time_period(1ms);

	volatile bool stop = false; // temporary, prevent optimisation
	auto previous = std::chrono::steady_clock::now() - UPDATE_DELTA;

	while(!stop) {
		const auto begin = std::chrono::steady_clock::now();
		auto delta = begin - previous;
		auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta);

		update(delta_ms);

		const auto end = std::chrono::steady_clock::now();
		const auto update_time = end - begin;

		if(update_time < UPDATE_DELTA) {
			std::this_thread::sleep_for(UPDATE_DELTA - update_time);
		}

		previous = begin;
	}
}

} // world, ember