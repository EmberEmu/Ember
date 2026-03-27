/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cstdint>
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif

namespace ember::utility {

std::uint64_t get_tick_count() {
#ifdef _WIN32
	return GetTickCount64();
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
	std::uint64_t tick = ts.tv_nsec / 1000000ull;
	tick += ts.tv_sec * 1000ull;
	return tick;
#endif
}

} // utility, ember