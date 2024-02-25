/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ThreadPool.h"
#include <boost/assert.hpp>

namespace ember {

using namespace std::string_literals;

ThreadPool::ThreadPool(std::size_t initial_count) : work_(service_), stopped_(false) {
	for(std::size_t i = 0; i < initial_count; ++i) {
		workers_.emplace_back(static_cast<std::size_t(boost::asio::io_context::*)()>
			(&boost::asio::io_context::run), &service_); 
	}
}


void ThreadPool::shutdown() {
	stopped_ = true;
	service_.stop();

	for(auto& worker : workers_) {
		try { // todo move when VS201? is out - apparently they didn't fix this in VS2015 either
			worker.join();
		} catch(const std::exception& e) {
			BOOST_ASSERT_MSG(false, e.what());

			std::lock_guard<std::mutex> guard(log_cb_lock_);

			if(log_cb_) {
				log_cb_(Severity::FATAL, "In thread pool shutdown: "s + e.what());
			}
		}
	}

}

ThreadPool::~ThreadPool() {
	if(!stopped_) {
		shutdown();
	}
}

void ThreadPool::log_callback(const LogCallback& callback) {
	std::lock_guard<std::mutex> guard(log_cb_lock_);
	log_cb_ = callback;
}

} //ember