/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Connection.h"
#include "PoolManager.h"
#include "../Spinlock.h"
#include "Policies.h"
#include <utility>
#include <functional>
#include <future>
#include <list>
#include <cstddef>
#include <exception>

namespace ember { namespace connection_pool {

template<typename Driver, typename ReusePolicy, typename GrowthPolicy>
class Pool : private ReusePolicy, private GrowthPolicy {
	template<typename ConType, typename Driver, typename ReusePolicy, typename GrowthPolicy>
	friend class PoolManager;

	using ConType = decltype(std::declval<Driver>().open());
	using ReusePolicy::return_clean;
	using GrowthPolicy::grow;

	PoolManager<ConType, Driver, ReusePolicy, GrowthPolicy> manager_;
	Driver driver_;
	std::size_t min_, max_;
	mutable Spinlock lock_;
	std::list<ConnDetail<ConType>> pool_;

	void open_connections(std::size_t num)  {
		std::vector<std::future<ConType>> futures;

		for (std::size_t i = 0; i < num; ++i) {
			auto f = std::async([](Driver& driver) {
				return driver.open();
			}, driver_);
			futures.emplace_back(std::move(f));
		}

		for (auto& f : futures) {
			pool_.emplace_back(ConnDetail<ConType>(f.get()));
		}
	}
	
public:
	Pool(Driver& driver, std::size_t min_size, std::size_t max_size)
	     : driver_(driver), min_(min_size), max_(max_size), manager_(this) {
		open_connections(min_);
		manager_.start();
	}

	~Pool() {
		manager_.stop();

		for(auto& c : pool_) {
			try {
				driver_.close(c.conn);
			} catch (...) { /* stop escapees */ }
		}
	}

	Connection<ConType> get_connection() {
		manager_.check_exceptions();

		auto pred = [](ConnDetail<ConType>& cd) {
			if(!cd.checked_out && !cd.dirty && !cd.error) {
				cd.checked_out = true;
				cd.idle = 0;
				return true;
			}
			return false;
		};

		std::unique_lock<Spinlock> guard(lock_);
		auto res = std::find_if(pool_.begin(), pool_.end(), pred);

		if (res == pool_.end()) {
			open_connections(grow(size(), max_));
			res = std::find_if(pool_.begin(), pool_.end(), pred);
		}

		guard.unlock();
		driver_.thread_enter();

		return Connection<ConType>(std::bind(&Pool::return_connection,
			this, std::placeholders::_1), *res);
	}

	void return_connection(Connection<ConType>& connection) {
		if(return_clean()) {
			connection.detail_.conn = driver_.clean(connection.detail_.conn);
		} else {
			connection.detail_.dirty = true;
		}

		connection.released_ = true;
		connection.detail_.checked_out = false;
		driver_.thread_exit();
		manager_.check_exceptions();
	}

	std::size_t size() {
		return pool_.size();
	}

	bool dirty() {
		return std::count_if(pool_.begin(), pool_.end(),
			[](const ConnDetail<ConType>& c) { return c.dirty; });
	}

	bool checked_out() {
		return std::count_if(pool_.begin(), pool_.end(),
			[](const ConnDetail<ConType>& c) { return c.checked_out; });
	}
};

}} //connection_pool, ember