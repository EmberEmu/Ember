/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <thread/ThreadPool.h>
#include <thread/Utility.h>

namespace ember::thread {

ThreadPool::ThreadPool(std::size_t initial_count)
	: work_(boost::asio::make_work_guard(service_)) {
	workers_.reserve(initial_count);

	for(std::size_t i = 0; i < initial_count; ++i) {
		workers_.emplace_back(&boost::asio::io_context::run, &service_);
		thread::set_name(workers_[i], "Thread Pool");
	}
}

void ThreadPool::shutdown() {
	service_.stop();
}

ThreadPool::~ThreadPool() {
	shutdown();
}

} // thread, ember