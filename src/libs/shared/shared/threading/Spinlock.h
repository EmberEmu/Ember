/*
 * Copyright (c) 2014 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define ember_x86_or_x64
#elif defined(__arm__) || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC) || defined(__aarch64__) || defined(_M_ARM64)
#define ember_arm
#endif

#include <atomic>
#include <thread>
#ifdef _MSC_VER
#include <intrin.h>
#elif defined (ember_x86_or_x64)
#include <x86intrin.h>
#endif

namespace ember {

class Spinlock final {
	static constexpr auto SPIN_COUNT { 16 };
	enum class State { LOCKED, UNLOCKED };
	std::atomic<State> state;

public:
	Spinlock() : state(State::UNLOCKED) {}

	inline bool acquire() {
		// relaxed load before attempting the exchange helps is friendlier to shared cache
		if(state.load(std::memory_order_relaxed) == State::LOCKED) {
			return false;
		}

		return state.exchange(State::LOCKED, std::memory_order_acquire) == State::UNLOCKED;
	}

	void lock() {
		for(auto spins = 0; !acquire(); ++spins) {
			if(spins == SPIN_COUNT) {
				spins = 0;
				std::this_thread::yield();
			} else {
#ifdef ember_x86_or_x64
				_mm_pause();
#elif defined(ember_arm)
				__yield();
#endif
			}
		}
	}

	void unlock() {
		state.store(State::UNLOCKED, std::memory_order_release);
	}
};

} // ember