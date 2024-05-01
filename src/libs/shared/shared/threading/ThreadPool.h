/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <cstddef>

#undef ERROR

namespace ember {

class ThreadPool {
public:
	enum class Severity { DEBUG, INFO, ERROR, FATAL };
	using LogCallback = std::function<void(Severity, std::string)>;

private:
	boost::asio::io_context service_;
	boost::asio::io_context::work work_;
	std::vector<std::thread> workers_;
	LogCallback log_cb_;
	std::mutex log_cb_lock_;
	bool stopped_;

public:
	explicit ThreadPool(std::size_t initial_count);
	~ThreadPool();

	template<typename T>
	void run(T work) {
#ifdef DEBUG_NO_THREADS
		work();
#else
		service_.post(work);
#endif
	}

	void shutdown();
	void log_callback(const LogCallback& callback);
};

} // ember
