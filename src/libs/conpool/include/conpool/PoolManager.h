/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Policies.h"
#include "Connection.h"
#include "ConnectionPool.h"
#include "LogSeverity.h"
#include <shared/threading/Spinlock.h>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <exception>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <cstddef>

namespace ember::connection_pool {

namespace sc = std::chrono;
using namespace std::chrono_literals;
using namespace std::string_literals;

template<typename Driver, typename ReusePolicy, typename GrowthPolicy> class Pool;

template<typename ConType, typename Driver, typename ReusePolicy, typename GrowthPolicy>
class PoolManager final {
	using ConnectionPool = Pool<Driver, ReusePolicy, GrowthPolicy>*;
	ConnectionPool pool_;
	Spinlock exception_lock_;
	sc::seconds interval_, max_idle_;
	std::thread manager_;
	std::exception_ptr exception_;
	std::condition_variable cond_;
	std::mutex cond_lock_;
	std::atomic_bool stop_ { false };

	void close(ConnDetail<ConType>& conn) {
		try {
			pool_->driver_.close(conn.conn);
		} catch(const std::exception& e) { 
			if(pool_->log_cb_) {
				pool_->log_cb_(Severity::WARN, "Connection close, driver threw: "s + e.what());
			}
		}
			
		conn.reset();
		std::atomic_thread_fence(std::memory_order_release);
		pool_->pool_guards_[conn.id].store(true, std::memory_order_relaxed);
		--pool_->size_;
	}

	void refresh(ConnDetail<ConType>& conn) {
		try {
			conn.error = !pool_->driver_.keep_alive(conn.conn);
			conn.idle = 0s;
		} catch(const std::exception& e) { 
			conn.error = true;

			if(pool_->log_cb_) {
				pool_->log_cb_(Severity::WARN, "Connection keep-alive, driver threw: "s + e.what());
			}
		}

		conn.refresh = false;
		conn.checked_out = false;
		std::atomic_thread_fence(std::memory_order_release);
		pool_->pool_guards_[conn.id].store(false, std::memory_order_relaxed);

		if(!conn.error) {
			pool_->semaphore_.release();
		}
	}

	// If the pool has grown too small, try to refill it
	void refill() {
		std::unique_lock<Spinlock> guard(pool_->lock_);

		if(pool_->size_ < pool_->min_) {
			try {
				pool_->open_connections(pool_->min_ - pool_->size_);
			} catch(const std::exception& e) { 
				guard.unlock();

				if(pool_->log_cb_) {
					pool_->log_cb_(Severity::WARN, "Failed to refill connection pool: "s + e.what());
				}
			}
		}
	}

	void maintain_connections() {
		for(auto& conn : pool_->pool_) {
			if(conn.empty_slot) {
				continue;
			} else if(conn.sweep || conn.error) {
				close(conn);
			} else if(conn.refresh) {
				refresh(conn);
			}
		}

		refill();
	}

	void set_connection_flags() {
		std::lock_guard<Spinlock> guard(pool_->lock_);
		std::size_t excess_connections = 0;

		if(pool_->size_ > pool_->min_) {
			excess_connections = pool_->pool_.size() - pool_->min_;
		}

		for(auto& conn : pool_->pool_) {
			if(conn.checked_out || conn.empty_slot) {
				continue;
			}

			if(conn.idle < max_idle_) {
				conn.idle += interval_;
			} else if(excess_connections > 0) {
				--excess_connections;
				conn.checked_out = true;
				conn.sweep = true;
			} else {
				conn.checked_out = true;
				conn.refresh = true;
			}

			std::atomic_thread_fence(std::memory_order_release);
			pool_->pool_guards_[conn.id].store(conn.checked_out, std::memory_order_relaxed);
		}
	}

	void manage_pool() {
		set_connection_flags();
		maintain_connections();
	}

public:
	PoolManager(ConnectionPool pool) {
		pool_ = pool;
	}

	void check_exceptions() {
		std::lock_guard<Spinlock> lock(exception_lock_);

		if(exception_) {
			std::rethrow_exception(exception_);
		}
	}

	void run() try {
		pool_->driver_.thread_enter();
		std::unique_lock<std::mutex> lock(cond_lock_);

#ifndef DEBUG_NO_THREADS
		while(!stop_) {
			cond_.wait_for(lock, interval_);
			manage_pool();
		}
#else
		manage_pool();
#endif

		pool_->driver_.thread_exit();
	} catch(...) {
		std::lock_guard<Spinlock> lock(exception_lock_);
		exception_ = std::current_exception();

		if(pool_->log_cb_) {
			pool_->log_cb_(Severity::DEBUG, "Pool manager trapped exception - passing to next caller");
		}
	}

	void stop() {
		if(manager_.joinable()) {
			stop_ = true;
			cond_.notify_one();
			manager_.join();
		}

		if(pool_->log_cb_) {
			pool_->log_cb_(Severity::DEBUG, "Pool manager terminated");
		}
	}

	void start(sc::seconds interval, sc::seconds max_idle) {
		interval_ = interval;
		max_idle_ = max_idle;
#ifndef DEBUG_NO_THREADS
		manager_ = std::thread(&PoolManager::run, this);
#endif
	}
};

} // connection_pool, ember
