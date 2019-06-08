/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Affinity.h"
#include <shared/CompilerWarn.h>
#include <stdexcept>
#include <string>

#ifdef _WIN32
	#include <Windows.h>
#elif defined TARGET_OS_MAC
	// todo
#elif defined __linux__ || defined __unix__
	#include <sched.h>
	#include <pthread.h>
#endif

namespace ember {

void set_affinity(std::thread& thread, unsigned int core) {	
#ifdef _WIN32
	if(SetThreadAffinityMask(thread.native_handle(), 1u << core) == 0) {
		throw std::runtime_error("Unable to set thread affinity, error code " + std::to_string(GetLastError()));
	}
#elif defined TARGET_OS_MAC
	// todo, no support for pthread_setaffinity_np - implement when I'm in a position to test it
#elif defined __linux__ || defined __unix__
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(core, &mask);
	auto ret = pthread_setaffinity_np(thread.native_handle(), sizeof(mask), &mask);

	if(ret) {
		throw std::runtime_error("Unable to set thread affinity, error code " + std::to_string(ret));
	}
#else
	#pragma message WARN("Setting thread affinity is not implemented for this platform. Implement it, please!");
#endif
}

} // ember