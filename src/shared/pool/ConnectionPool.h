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
#include "Spinlock.h"
#include "Policies.h"
#include <utility>
#include <functional>
#include <future>
#include <list>
#include <cstddef>

namespace ConnectionPool {

template<typename Driver, typename ReusePolicy, typename GrowthPolicy>
class Pool : private ReusePolicy, private GrowthPolicy {
	template<typename ConType> friend class PoolManager;

	using ConType = decltype(std::declval<Driver>().open());
	using ReusePolicy::return_clean;
	using GrowthPolicy::grow;

	Driver driver;
	std::size_t min, max;
	mutable Spinlock lock;
	std::list<ConnDetail<ConType>> pool;

	void open_connections(std::size_t num)  {
		std::vector<std::future<ConType>> futures;

		for (std::size_t i = 0; i < num; ++i) {
			auto f = std::async([](Driver& driver) {
				return driver.open();
			}, driver);
			futures.emplace_back(std::move(f));
		}

		for (auto& f : futures) {
			pool.emplace_back(ConnDetail<ConType>(f.get()));
		}
	}
	
public:
	Pool(const Driver& driver, std::size_t min_size, std::size_t max_size)
	     : driver(driver), min(min_size), max(max_size), manager(this) {
		open_connections(min);
	}

	~Pool() {
		for(auto& c : pool) {
			try {
				driver.close(c.conn);
			} catch (...) { /* stop escapees */ }
		}
	}

	Connection<ConType> get_connection() {
		auto pred = [](ConnDetail<ConType>& cd) {
			if(!cd.checked_out && !cd.dirty && !cd.error) {
				cd.checked_out = true;
				cd.idle = 0;
				return true;
			}
			return false;
		};

		std::unique_lock<Spinlock> guard(lock);
		auto res = std::find_if(pool.begin(), pool.end(), pred);

		if(res == pool.end()) {
			open_connections(grow(size(), max));
			res = std::find_if(pool.begin(), pool.end(), pred);
		}

		guard.unlock();
		driver.thread_enter();

		return Connection<ConType>(std::bind(&Pool::return_connection,
			this, std::placeholders::_1), *res);
	}

	void return_connection(Connection<ConType>& connection) {
		if(return_clean()) {
			connection.detail.conn = driver.clean(connection.detail.conn);
		} else {
			connection.detail.dirty = true;
		}

		connection.released = true;
		connection.detail.checked_out = false;
		driver.thread_exit();
	}

	auto size() {
		return pool.size();
	}

	auto dirty() {
		return std::count_if(pool.begin(), pool.end(),
			[](const ConnDetail<ConType>& c) { return c.dirty; });
	}

	auto checked_out() {
		return std::count_if(pool.begin(), pool.end(),
			[](const ConnDetail<ConType>& c) { return c.checked_out; });
	}
};

} //ConnectionPool