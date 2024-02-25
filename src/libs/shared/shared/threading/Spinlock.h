/*
 * Copyright (c) 2014 - 2022 Ember
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

class Spinlock {
	enum class State { LOCKED, UNLOCKED };
	std::atomic<State> state;

public:
	Spinlock() : state(State::UNLOCKED) {}

	void lock() {
		while(true) {
			if(state.exchange(State::LOCKED, std::memory_order_acquire) == State::UNLOCKED) {
				return;
			}

			for(int i = 0; i < 1000; ++i) {
				_mm_pause();

				if(state.load(std::memory_order_relaxed) == State::UNLOCKED) {
					break;
				}
			}

			std::this_thread::yield();
		}
	}

	void unlock() {
		state.store(State::UNLOCKED, std::memory_order_release);
	}
};

} //Ember