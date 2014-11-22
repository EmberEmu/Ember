/*
* Copyright (c) 2014 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <atomic>

namespace ember {

class Spinlock {
	typedef enum { LOCKED, UNLOCKED } State;
	std::atomic<State> state;

public:
	Spinlock() : state(UNLOCKED) {}

	void lock() {
		while(true) {
			for(int i = 0; i < 1000; ++i) {
				if(state.exchange(LOCKED, std::memory_order_acquire) == UNLOCKED) {
					return;
				}
			}
			std::this_thread::yield();
		}
	}

	void unlock() {
		state.store(UNLOCKED, std::memory_order_release);
	}
};

} //Ember