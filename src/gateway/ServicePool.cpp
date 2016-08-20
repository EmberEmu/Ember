/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ServicePool.h"
#include <shared/threading/Affinity.h>
#include <stdexcept>

namespace ember {

ServicePool::ServicePool(std::size_t pool_size) : pool_size_(pool_size), next_service_(0) {
	if(pool_size == 0) {
		throw std::runtime_error("Cannot have an empty ASIO IO service pool!");
	}

	for(std::size_t i = 0; i < pool_size; ++i) {
		auto io_service = std::make_shared<boost::asio::io_service>();
		auto work = std::make_shared<boost::asio::io_service::work>(*io_service);
		services_.emplace_back(io_service);
		work_.emplace_back(work);
	}
}

boost::asio::io_service& ServicePool::get_service() {
	auto& service = *services_[next_service_++];
	next_service_ %= pool_size_;
	return service;
}

void ServicePool::run() {
	std::vector<std::thread> threads;
	auto core_count = std::thread::hardware_concurrency();

	for(std::size_t i = 0; i < pool_size_; ++i) {
		threads.emplace_back(static_cast<std::size_t(boost::asio::io_service::*)()>
			(&boost::asio::io_service::run), services_[i]);
		set_affinity(threads[i], i % core_count);
	}

	// blocks until the worker threads exit
	for(auto& thread : threads) {
		thread.join();
	}
}

void ServicePool::stop() {
	work_.clear();

 	for(auto& service : services_) {
		service->stop();
	}
}

std::size_t ServicePool::size() const {
	return pool_size_;
}

} // ember