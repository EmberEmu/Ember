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
	unsigned int count_;
	std::condition_variable condition_;
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

	void wait() {
		std::unique_lock<Lock> guard(lock_);

		while(!count_) {
			condition_.wait(guard);
		}

		--count_;
	}

	bool wait_for(std::chrono::milliseconds duration) {
		std::unique_lock<Lock> guard(lock_);

		//wait_for with a predicate is broken in VS2013 & VS2015 Preview - workaround!
		//todo, submitted bug report, has now been fixed for VS2015 RTM
		bool hack = false;

		condition_.wait_for(guard, std::chrono::milliseconds(duration),
			[this, &hack]() {
				if(count_ != 0) {
					--count_;
					hack = true;
					return true;
				}
				return false;
			});

		return hack;
	}

	void signal(unsigned int increment = 1) {
		std::lock_guard<Lock> guard(lock_);
		increment_max_check(increment); 
		condition_.notify_one();
	}

	void signal_all(unsigned int increment = 1) {
		std::lock_guard<Lock> guard(lock_);
		increment_max_check(increment);
		condition_.notify_all();
	}
};

} //ember