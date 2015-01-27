/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ThreadPool.h"

namespace ember {

ThreadPool::ThreadPool(std::size_t initial_count) : work_(service_) {
	for(std::size_t i = 0; i < initial_count; ++i) {
		workers_.emplace_back(std::bind(&ThreadPool::run_catch, this));
	}
}

ThreadPool::~ThreadPool() {
	service_.stop();

	for(auto& worker : workers_) {
		try { //todo move when VS2015 is out - VS2013 bug
			worker.join();
		} catch(std::exception& e) {
			std::lock_guard<std::mutex> guard(log_cb_lock_);
			if(log_cb_) {
				log_cb_(SEVERITY::ERROR, std::string("In thread pool dtor: ") + e.what());
			}
		} catch(...) {
			std::lock_guard<std::mutex> guard(log_cb_lock_);
			if(log_cb_) {
				log_cb_(SEVERITY::ERROR, "Caught an unknown exception in thread pool dtor");
			}
		}
	}
}

void ThreadPool::run_catch() {
	while(true) try {
		service_.run();
		break; //io_service exited normally
	} catch(std::exception& e) {
		std::lock_guard<std::mutex> guard(log_cb_lock_);
		if(log_cb_) {
			log_cb_(SEVERITY::ERROR, std::string("Task threw an exception: ") + e.what());
		}
	} catch(...) {
		std::lock_guard<std::mutex> guard(log_cb_lock_);
		if(log_cb_) {
			log_cb_(SEVERITY::ERROR, "Unknown exception thrown during task execution");
		}
	}
}

void ThreadPool::log_callback(LogCallback callback) {
	std::lock_guard<std::mutex> guard(log_cb_lock_);
	log_cb_ = callback;
}

} //ember