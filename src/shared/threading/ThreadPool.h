/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/io_service.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ember {

#undef ERROR 

class ThreadPool {
public:
	enum class SEVERITY { DEBUG, INFO, ERROR, FATAL };
	typedef std::function<void(SEVERITY, std::string)> LogCallback;

private:
	boost::asio::io_service service_;
	boost::asio::io_service::work work_;
	std::vector<std::thread> workers_;
	std::size_t count_;
	LogCallback log_cb_;
	std::mutex log_cb_lock_;
	void run_catch();

public:
	ThreadPool(std::size_t initial_count);
	~ThreadPool();

	template<typename T> void run(T work) {
		service_.post(work);
	}

	void log_callback(const LogCallback& callback);
};

} //ember