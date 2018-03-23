/*
 * Copyright (c) 2016 - 2018 Ember
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

ServicePool::~ServicePool() {
	stop();
}

boost::asio::io_service& ServicePool::get_service() {
	auto& service = *services_[next_service_++];
	next_service_ %= pool_size_;
	return service;
}

boost::asio::io_service* ServicePool::get_service(std::size_t index) const {
	if(index >= services_.size()) {
		return nullptr;
	}

	return services_[index].get();
}

void ServicePool::run() {
	const auto core_count = std::thread::hardware_concurrency();

	for(std::size_t i = 0; i < pool_size_; ++i) {
		threads_.emplace_back(static_cast<std::size_t(boost::asio::io_service::*)()>
			(&boost::asio::io_service::run), services_[i]);
		set_affinity(threads_[i], i % core_count);
	}
}

void ServicePool::stop() {
	work_.clear();

 	for(auto& service : services_) {
		if(!service->stopped()) {
			service->stop();
		}
	}

	for(auto& thread : threads_) {
		if(thread.joinable()) {
			thread.join();
		}
	}
}

std::size_t ServicePool::size() const {
	return pool_size_;
}

} // ember