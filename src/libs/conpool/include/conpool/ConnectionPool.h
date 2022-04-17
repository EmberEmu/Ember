/*
 * Copyright (c) 2014 - 2022 Ember
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
#include "LogSeverity.h"
#include <shared/threading/Spinlock.h>
#include <boost/assert.hpp>
#include <optional>
#include <utility>
#include <functional>
#include <future>
#include <list>
#include <exception>
#include <string>
#include <mutex>
#include <chrono>
#include <atomic>
#include <vector>
#include <semaphore>
#include <cstddef>

namespace ember::connection_pool {

namespace sc = std::chrono;
using namespace std::chrono_literals;
using namespace std::string_literals;

template<typename ConType, typename Driver, typename ReusePolicy, typename GrowthPolicy> class PoolManager;

template<typename Driver, typename ReusePolicy, typename GrowthPolicy>
class Pool : private ReusePolicy, private GrowthPolicy {
	template<typename, typename, typename, typename>
	friend class PoolManager;

	using ConType = decltype(std::declval<Driver>().open());
	using ReusePolicy::return_clean;
	using GrowthPolicy::grow;

	PoolManager<ConType, Driver, ReusePolicy, GrowthPolicy> manager_;
	Driver& driver_;
	const std::size_t min_, max_;
	std::atomic<std::size_t> size_;
	Spinlock lock_;
	std::vector<ConnDetail<ConType>> pool_;
	std::vector<std::atomic<bool>> pool_guards_;

	std::counting_semaphore<1024> semaphore_ {0};
	std::function<void(Severity, std::string)> log_cb_;
	std::atomic_bool closed_;

	void set_connection_ids() {
		unsigned int connection_id = 0;

		for(auto& connection : pool_) {
			connection.id = connection_id;
			++connection_id;
		}
	}

	void open_connections(std::size_t num)  {
		std::vector<std::future<ConType>> futures;

		for(std::size_t i = 0; i < num; ++i) {
			auto f = std::async(std::launch::async, [](Driver* driver) {
				return driver->open();
			}, &driver_);

			futures.emplace_back(std::move(f));
		}

		auto pool_it = pool_.begin();

		for(auto& f : futures) {
			while(!pool_it->empty_slot) {
				++pool_it;
				BOOST_ASSERT_MSG(pool_it != pool_.end(), "Exceeded maximum database connection count.");
			}

			*pool_it = std::move(ConnDetail<ConType>(f.get(), pool_it->id));
			++size_;
			semaphore_.release();
		}
	}

	bool find_free_connection(ConnDetail<ConType>& cd) {
		bool checked_out = pool_guards_[cd.id].load(std::memory_order_relaxed);

		if(checked_out) {
			return false;
		}

		std::atomic_thread_fence(std::memory_order_acquire);

		if(!cd.error && !cd.sweep && !cd.empty_slot) {
			if(cd.dirty && !return_clean()) {
				try {
					if(driver_.clean(cd.conn)) {
						cd.dirty = false;
					} else {
						cd.sweep = true;
						return false;
					}
				} catch(const std::exception& e) {
					if(log_cb_) {
						log_cb_(Severity::DEBUG, "On connection clean: "s + e.what());
					}
					return false;
				}
			}

			cd.checked_out = true;
			cd.idle = 0s;
			pool_guards_[cd.id].store(true, std::memory_order_relaxed);
			return true;
		}

		return false;
	}
	
	std::optional<Connection<ConType>> get_connection_attempt() {
#ifdef DEBUG_NO_THREADS
		manager_.run();
#endif
		manager_.check_exceptions();

		std::unique_lock<Spinlock> guard(lock_);

		auto res = std::find_if(pool_.begin(), pool_.end(), [&](auto& arg) {
			return this->find_free_connection(arg); // this-> is a GCC bug workaround
		});

		if(res == pool_.end()) {
			open_connections(grow(size(), max_));

			res = std::find_if(pool_.begin(), pool_.end(), [&](auto& arg) {
				return this->find_free_connection(arg);
			});
			
			if(res == pool_.end()) {
				return std::nullopt;
			}
		}

		guard.unlock();
		driver_.thread_enter();

		return Connection<ConType>([this](Connection<ConType>& arg) {
			this->return_connection(arg);
		}, *res);
	}
	
public:
	Pool(Driver& driver, std::size_t min_size, std::size_t max_size,
	     sc::seconds max_idle, sc::seconds interval = 15s)
	     : driver_(driver), min_(min_size), max_(max_size), manager_(this), pool_(max_size),
		   pool_guards_(max_size), size_(0), closed_(false) {

		if(!max_size) {
			throw exception("Max. database connections cannot be zero");
		}

		if(min_size > max_size) {
			throw exception("Min. database connections must be <= max.");
		}

		set_connection_ids();
		open_connections(min_);
		manager_.start(interval, max_idle);
	}

	~Pool() {
		if(closed_) {
			return;
		}

		manager_.stop();

		for(auto& c : pool_) {
			if(c.empty_slot) {
				continue;
			}

			BOOST_ASSERT_MSG(!c.checked_out, "Closed connection pool without returning all connections.");

			try {
				driver_.close(c.conn);
			} catch(const std::exception& e) { 
				if(log_cb_) {
					log_cb_(Severity::WARN, "Closing pool, driver threw: "s + e.what());
				}
			}
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
			if(c.empty_slot) {
				continue;
			}

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
		std::optional<Connection<ConType>> conn(get_connection_attempt());

		if(!conn) {
			throw no_free_connections();
		}

		return std::move(conn.get());
	}

	/*
	 * This function checks for a connection in a loop because being woken up
	 * from acquire doesn't guarantee that a connection will be available for the
	 * thread, only that a connection has been added to the pool/made available
	 * for reuse.
	 */
	Connection<ConType> wait_connection() {
		std::optional<Connection<ConType>> conn;
		
		while(!(conn = get_connection_attempt())) {
			semaphore_.acquire();
		}

		return std::move(conn.get());
	}

	/*
	 * This function handles its own timing because being woken up from try_acquire_for
	 * doesn't guarantee that a connection will be available for the thread,
	 * only that a connection has been added to the pool/made available for reuse.
	 * Therefore, if the thread is woken up, it will check for a connection. If none
	 * are available, it will wait for the next notification and keep trying until
	 * either the time has elapsed or it manages to get a connection.
	 */
	Connection<ConType> wait_connection(std::chrono::milliseconds duration) {
		auto start = sc::high_resolution_clock::now();
		std::optional<Connection<ConType>> conn;

		while(!(conn = get_connection_attempt())) {
			sc::milliseconds elapsed = sc::duration_cast<sc::milliseconds>
				(sc::high_resolution_clock::now() - start);
			
			if(elapsed >= duration) {
				throw no_free_connections();
			}

			// If no connections are added while we're waiting, give up
			if(!semaphore_.try_acquire_for(duration - elapsed)) {
				throw no_free_connections();
			}
		}

		return std::move(conn.value());
	}

	void return_connection(Connection<ConType>& connection) {
		auto& detail = connection.detail_.get();

		if(return_clean()) {
			if(!driver_.clean(detail.conn)) {
				detail.dirty = true;
				detail.sweep = true;
			}
		} else {
			detail.dirty = true;
		}

		connection.released_ = true;
		detail.checked_out = false;
		std::atomic_thread_fence(std::memory_order_release);
		pool_guards_[detail.id].store(false, std::memory_order_relaxed);

		driver_.thread_exit();
		manager_.check_exceptions();
		semaphore_.release();
	}

	std::size_t size() const {
		return size_;
	}

	void logging_callback(std::function<void(Severity, std::string)> callback) {
		log_cb_ = callback;
	}

	auto dirty() const {
		return std::count_if(pool_.begin(), pool_.end(),
			[](const ConnDetail<ConType>& c) { return c.dirty; });
	}

	auto checked_out() const {
		return std::count_if(pool_.begin(), pool_.end(),
			[](const ConnDetail<ConType>& c) { return c.checked_out; });
	}

	Driver* get_driver() const {
		return &driver_;
	}
};

} // connection_pool, ember