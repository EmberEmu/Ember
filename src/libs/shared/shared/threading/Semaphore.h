/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <condition_variable>
#include <chrono>
#include <mutex>

namespace ember {

/*
 * This semaphore predates the additions to std::* and it does
 * not behave the same. The main difference is that calling
 * release() when the counter is at its maximum value will
 * have no effect, which is not allowed with the std semaphores.
 */
template<typename Lock>
class Semaphore final {
	std::condition_variable condition_;
	unsigned int count_;
	unsigned int max_;
	Lock lock_;

	void increment_max_check(unsigned int increment) {
		unsigned int old = count_;
		count_ += increment;

		if(count_ < old || count_ > max_) { //check for overflow
			count_ = max_;
		}
	}

public:
	Semaphore(unsigned int initial_count = 0,
	          unsigned int max_count = std::numeric_limits<unsigned int>::max())
	          : count_(initial_count) {
		max_ = max_count;
	}

	void acquire() {
		std::unique_lock guard(lock_);

		while(!count_) {
			condition_.wait(guard);
		}

		--count_;
	}

	bool try_acquire_for(std::chrono::milliseconds duration) {
		std::unique_lock guard(lock_);

		return condition_.wait_for(guard, duration,
			[this]() {
				if(count_ != 0) {
					--count_;
					return true;
				}
				return false;
			});
	}

	void release(unsigned int increment = 1) {
		if(!increment) {
			return;
		}

		std::lock_guard guard(lock_);
		increment_max_check(increment);

		for(auto i = 0u; i < increment; ++i) {
			condition_.notify_one();
		}
	}

	unsigned int count() {
		return count_;
	}
};

} // ember