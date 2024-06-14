/*
 * Copyright (c) 2016 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ServicePool.h"
#include <shared/threading/Utility.h>
#include <utility>
#include <stdexcept>

namespace ember {

ServicePool::ServicePool(const std::size_t pool_size) : pool_size_(pool_size), next_service_(0) {
	if(pool_size == 0) {
		throw std::runtime_error("Cannot have an empty ASIO IO service pool!");
	}

	for(std::size_t i = 0; i < pool_size; ++i) {
		auto& ctx = services_.emplace_back(
			std::make_unique<boost::asio::io_context>(ASIO_CONCURRENCY_HINT)
		);
		work_.emplace_back(std::make_shared<boost::asio::io_context::work>(*ctx));
	}
}

ServicePool::~ServicePool() {
	stop();
}

boost::asio::io_context& ServicePool::get_service() {
	auto& service = *services_[next_service_++];
	next_service_ %= pool_size_;
	return service;
}

boost::asio::io_context* ServicePool::get_service(const std::size_t index) const {
	if(index >= services_.size()) {
		return nullptr;
	}

	return services_[index].get();
}

void ServicePool::run() {
	const auto core_count = std::thread::hardware_concurrency();

	for(std::size_t i = 0; i < pool_size_; ++i) {
		threads_.emplace_back(static_cast<std::size_t(boost::asio::io_context::*)()>
			(&boost::asio::io_context::run), services_[i].get());
		thread::set_affinity(threads_[i], i % core_count);
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