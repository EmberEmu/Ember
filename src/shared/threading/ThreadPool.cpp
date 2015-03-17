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

ThreadPool::ThreadPool(std::size_t initial_count) : work_(service_) {
	for(std::size_t i = 0; i < initial_count; ++i) {
		workers_.emplace_back(static_cast<std::size_t(boost::asio::io_service::*)()>
			(&boost::asio::io_service::run), &service_); 
	}
}

ThreadPool::~ThreadPool() {
	service_.stop();

	for(auto& worker : workers_) {
		try { //todo move when VS2015 is out - VS2013 bug
			worker.join();
		} catch(std::exception& e) {
			BOOST_ASSERT_MSG(false, e.what());

			std::lock_guard<std::mutex> guard(log_cb_lock_);

			if(log_cb_) {
				log_cb_(SEVERITY::FATAL, std::string("In thread pool dtor: ") + e.what());
			}
		}
	}
}

void ThreadPool::log_callback(const LogCallback& callback) {
	std::lock_guard<std::mutex> guard(log_cb_lock_);
	log_cb_ = callback;
}

} //ember