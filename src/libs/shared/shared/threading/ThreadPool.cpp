/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ThreadPool.h"
#include <shared/threading/Utility.h>

namespace ember {

ThreadPool::ThreadPool(std::size_t initial_count) : work_(service_) {
	for(std::size_t i = 0; i < initial_count; ++i) {
		workers_.emplace_back(static_cast<std::size_t(boost::asio::io_context::*)()>
			(&boost::asio::io_context::run), &service_);
		thread::set_name(workers_[i], "Thread Pool");
	}
}

void ThreadPool::shutdown() {
	service_.stop();
}

ThreadPool::~ThreadPool() {
	shutdown();
}

} // ember