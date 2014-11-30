/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <chrono>

namespace ember {

template<typename Lock>
class Semaphore {
	std::atomic<unsigned int> count_;
	std::condition_variable condition_;
	Lock lock_;

public:
	Semaphore(unsigned int initial_count = 0) {
		count_ = initial_count;
	}

	void wait() {
		std::unique_lock<Lock> guard(lock_);

		while(!count_) {
			condition_.wait(guard);
		}

		--count_;
	}

	bool wait_for(std::chrono::milliseconds duration) {
		std::unique_lock<Lock> guard(lock_);

		return !condition_.wait_for(guard, std::chrono::milliseconds(duration),
			[this]() {
				if(count_ != 0) {
					--count_;
					return true;
				}
				return false;
			});
	}

	void signal() {
		++count_;
		condition_.notify_one();
	}

	void signal_all() {
		++count_;
		condition_.notify_all();
	}
};

} //ember