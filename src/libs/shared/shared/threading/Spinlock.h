/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <atomic>
#include <thread>
#ifdef _MSC_VER
#include <intrin.h>
#else
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
		if(state.load(std::memory_order_relaxed) == State::LOCKED) {
			return false;
		}

		if(state.exchange(State::LOCKED, std::memory_order_acquire) == State::UNLOCKED) {
			return true;
		} else {
			return false;
		}
	}

	void lock() {
		for(auto spins = 0; !acquire(); ++spins) {
			if(spins == SPIN_COUNT) {
				spins = 0;
				std::this_thread::yield();
			} else {
				_mm_pause();
			}
		}
	}

	void unlock() {
		state.store(State::UNLOCKED, std::memory_order_release);
	}
};

} // ember