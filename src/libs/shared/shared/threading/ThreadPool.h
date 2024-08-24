/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <thread>
#include <utility>
#include <vector>
#include <cstddef>

namespace ember {

class ThreadPool final {
	boost::asio::io_context service_;
	boost::asio::io_context::work work_;
	std::vector<std::jthread> workers_;

public:
	explicit ThreadPool(std::size_t initial_count);
	~ThreadPool();

	template<typename T>
	void run(T&& work) {
#ifdef DEBUG_NO_THREADS
		work();
#else
		service_.post(std::move(work));
#endif
	}

	void shutdown();
};

} // ember
