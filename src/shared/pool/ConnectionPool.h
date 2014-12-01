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
#include "Policies.h"
#include "Exception.h"
#include "../threading/Spinlock.h"
#include "../threading/Semaphore.h"
#include <boost/optional.hpp>
#include <boost/assert.hpp>
#include <utility>
#include <functional>
#include <future>
#include <list>
#include <cstddef>
#include <exception>
#include <mutex>
#include <chrono>

namespace ember { namespace connection_pool {

namespace sc = std::chrono;

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
	Semaphore<std::mutex> semaphore_;
	bool closed_ = false;

	void open_connections(std::size_t num)  {
		std::vector<std::future<ConType>> futures;

		for(std::size_t i = 0; i < num; ++i) {
			auto f = std::async([](Driver& driver) {
				return driver.open();
			}, driver_);
			futures.emplace_back(std::move(f));
		}

		for(auto& f : futures) {
			pool_.emplace_back(ConnDetail<ConType>(f.get()));
			semaphore_.signal();
		}
	}
	
	boost::optional<Connection<ConType>> get_connection_attempt() {
		manager_.check_exceptions();

		auto pred = [](ConnDetail<ConType>& cd) {
			if(!cd.checked_out && !cd.dirty && !cd.error) {
				cd.checked_out = true;
				cd.idle = sc::seconds(0);
				return true;
			}
			return false;
		};

		std::unique_lock<Spinlock> guard(lock_);
		auto res = std::find_if(pool_.begin(), pool_.end(), pred);

		if(res == pool_.end()) {
			open_connections(grow(size(), max_));
			res = std::find_if(pool_.begin(), pool_.end(), pred);
			
			if(res == pool_.end()) {
				return boost::optional<Connection<ConType>>();
			}
		}

		guard.unlock();
		driver_.thread_enter();

		return Connection<ConType>(std::bind(&Pool::return_connection,
			this, std::placeholders::_1), *res);
	}
	
public:
	Pool(Driver& driver, std::size_t min_size, std::size_t max_size,
	     sc::seconds max_idle, sc::seconds interval = sc::seconds(10))
	     : driver_(driver), min_(min_size), max_(max_size), manager_(this) {
		open_connections(min_);
		manager_.start(interval, max_idle);
	}

	~Pool() {
		if(closed_) {
			return;
		}

		manager_.stop();

		for(auto& c : pool_) {
			BOOST_ASSERT_MSG(!c.checked_out, 
			                 "Closed connection pool without returning all connections.");

			try {
				driver_.close(c.conn);
			} catch (...) { /* stop escapees */ }
		}
	}

	void close() {
		if(closed_) {
			throw exception("Closed the same connection pool twice!");
		}

		closed_ = true;
		int active = 0;

		manager_.stop();

		for(auto& c : pool_) {
			if(c.checked_out) {
				++active;
				continue;
			}

			driver_.close(c.conn);
		}

		if(active) {
			throw active_connections(active);
		}
	}

	Connection<ConType> get_connection() {
		boost::optional<Connection<ConType>> conn(get_connection_attempt());

		if(!conn) {
			throw no_free_connections();
		}

		return std::move(conn.get());
	}

	/*
	 * This function checks for a connection in a loop because being woken up
	 * from wait_for doesn't guarantee that a connection will be available for the
	 * thread, only that a connection has been added to the pool/made available
	 * for reuse.
	 */
	Connection<ConType> wait_connection() {
		boost::optional<Connection<ConType>> conn;
		
		while(!(conn = get_connection_attempt())) {
			semaphore_.wait();
		}

		return std::move(conn.get());
	}

	/*
	 * This function handles its own timing because being woken up from wait_for
	 * doesn't guarantee that a connection will be available for the thread,
	 * only that a connection has been added to the pool/made available for reuse.
	 * Therefore, if the thread is woken up, it will check for a connection. If none
	 * are available, it will wait for the next notification and keep trying until
	 * either the time has elapsed or it manages to get a connection.
	 */
	Connection<ConType> wait_connection(std::chrono::milliseconds duration) {
		auto start = sc::high_resolution_clock::now();
		boost::optional<Connection<ConType>> conn;

		while(!(conn = get_connection_attempt())) {
			sc::milliseconds elapsed = sc::duration_cast<sc::milliseconds>
				(sc::high_resolution_clock::now() - start);
			
			if(elapsed >= duration) {
				throw no_free_connections();
			}

			semaphore_.wait_for(duration - elapsed);
		}

		return std::move(conn.get());
	}

	void return_connection(Connection<ConType>& connection) {
		if(return_clean()) {
			connection.detail_.get().conn = driver_.clean(connection.detail_.get().conn);
		} else {
			connection.detail_.get().dirty = true;
		}

		connection.released_ = true;
		connection.detail_.get().checked_out = false;

		driver_.thread_exit();
		manager_.check_exceptions();
		semaphore_.signal();
	}

	std::size_t size() {
		return pool_.size();
	}

	bool dirty() {
		return std::count_if(pool_.begin(), pool_.end(),
			[](const ConnDetail<ConType>& c) { return c.dirty; });
	}

	std::size_t checked_out() {
		return std::count_if(pool_.begin(), pool_.end(),
			[](const ConnDetail<ConType>& c) { return c.checked_out; });
	}
};

}} //connection_pool, ember