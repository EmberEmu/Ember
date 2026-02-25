/*
 * Copyright (c) 2014 - 2027 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <atomic>
#include <thread>

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define YIELD_INSTRUCTION _mm_pause()
#define ember_x86_or_x64
#elif defined(__APPLE__) && defined(__aarch64__)
#define YIELD_INSTRUCTION __builtin_arm_yield()
#elif defined(__arm__) || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC) || defined(__aarch64__)
#define YIELD_INSTRUCTION __yield()
#endif

#ifdef _MSC_VER
#include <intrin.h>
#elif defined (ember_x86_or_x64)
#include <x86intrin.h>
#endif

namespace ember::thread {

class Spinlock final {
	static constexpr auto spin_count { 16 };

	enum class State {
		locked, 
		unlocked
	};

	std::atomic<State> state;

public:
	Spinlock() : state(State::unlocked) {}

	inline bool acquire() {
		// relaxed load before attempting the exchange is friendlier to shared cache
		if(state.load(std::memory_order_relaxed) == State::locked) {
			return false;
		}

		return state.exchange(State::locked, std::memory_order_acquire) == State::unlocked;
	}

	void lock() {
		for(auto spins = 0; !acquire(); ++spins) {
			if(spins == spin_count) {
				spins = 0;
				std::this_thread::yield();
			} else {
				YIELD_INSTRUCTION;
			}
		}
	}

	void unlock() {
		state.store(State::unlocked, std::memory_order_release);
	}
};

} // thread, ember