/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <thread/ServicePool.h>
#include <thread/Utility.h>
#include <utility>
#include <stdexcept>

namespace ember::thread {

ServicePool::ServicePool(const std::size_t pool_size, const int hint)
	: hint_(hint)
	, pool_size_(limit_pool_size(pool_size))
	, next_service_(0)  {
	initialise_pool();
}

ServicePool::~ServicePool() {
	stop();
}

std::size_t ServicePool::limit_pool_size(const std::size_t pool_size) {
	if(pool_size == 0) {
		throw std::runtime_error("Cannot have an empty Asio io_context pool");
	}
	
	return pool_size;
}

void ServicePool::initialise_pool() {
	for(std::size_t i = 0; i < pool_size_; ++i) {
		auto& ctx = services_.emplace_back(
			std::make_unique<boost::asio::io_context>(hint_)
		);

		work_.emplace_back(boost::asio::make_work_guard(*ctx));
	}
}

boost::asio::io_context& ServicePool::get() {
	auto& service = *services_[next_service_++];
	next_service_ %= pool_size_;
	return service;
}

boost::asio::io_context& ServicePool::get(const std::size_t index) const {
	if(index >= services_.size()) {
		throw std::out_of_range("Bad service index specified");
	}

	return *services_[index].get();
}

boost::asio::io_context* ServicePool::get_if(const std::size_t index) const {
	if(index >= services_.size()) {
		return nullptr;
	}

	return services_[index].get();
}

void ServicePool::run() {
	if(!threads_.empty()) {
		throw std::runtime_error("Service pool already running");
	}

	const auto core_count = std::thread::hardware_concurrency();

	for(std::size_t i = 0; i < pool_size_; ++i) {
		threads_.emplace_back(&boost::asio::io_context::run, services_[i].get());
		thread::set_affinity(threads_[i], i % (core_count? core_count : 1));
		thread::set_name(threads_[i], "Service Pool");
	}
}

// only returns once all pending work has been completed 
void ServicePool::shutdown() {
	work_.clear();
	threads_.clear();
}

void ServicePool::stop() {
 	for(auto& service : services_) {
		if(!service->stopped()) {
			service->stop();
		}
	}

	work_.clear();
	threads_.clear();
}

std::size_t ServicePool::size() const {
	return pool_size_;
}

} // thread, ember