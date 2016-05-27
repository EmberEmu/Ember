/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/threading/Spinlock.h>
#include <atomic>
#include <deque> // todo
#include <mutex>
#include <cstddef>

namespace ember { namespace task { namespace ws {

struct Task;
// todo, etc
class Dequeue {
	std::deque<Task*> container_; // temp
	std::mutex lock_;
	std::atomic<std::size_t> size_;

public:
	Task* try_steal() {
		std::lock_guard<std::mutex> guard(lock_);

		if(container_.empty()) {
			return nullptr;
		}

		Task* task = container_.front();
		container_.pop_front();
		return task;
	}

	Task* try_pop_back() {
		std::lock_guard<std::mutex> guard(lock_);

		if(container_.empty()) {
			return nullptr;
		}

		Task* task = container_.back();
		container_.pop_back();
		return task;
	}

	void push_back(Task* task) {
		std::lock_guard<std::mutex> guard(lock_);
		container_.push_back(task);
	}

	std::size_t size() {
		return size_;
	}
};

}}} // ws, task, ember