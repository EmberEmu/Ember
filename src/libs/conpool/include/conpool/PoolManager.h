/*
 * Copyright (c) 2014 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <conpool/Policies.h>
#include <conpool/Connection.h>
#include <conpool/PoolImpl.h>
#include <conpool/LogSeverity.h>
#include <thread/Spinlock.h>
#include <thread/Utility.h>
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

#define POOL_LOG(sev, msg)               \
if(pool_->logger_) {                     \
	pool_->logger_(sev, std::move(msg)); \
}

namespace ember::connection_pool {

namespace sc = std::chrono;
using namespace std::chrono_literals;
using namespace std::string_literals;

template<typename Driver, typename ReusePolicy, typename GrowthPolicy, unsigned int> class PoolImpl;

template<typename ConType, typename Driver, typename ReusePolicy, typename GrowthPolicy, unsigned int size_hint>
class PoolManager final {
	using ConnectionPool = PoolImpl<Driver, ReusePolicy, GrowthPolicy, size_hint>*;
	ConnectionPool pool_;
	thread::Spinlock exception_lock_;
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
			POOL_LOG(Severity::error, "Connection close, driver threw: "s + e.what());
		}

		conn = ConnDetail<ConType>();
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
			POOL_LOG(Severity::error, "Connection keep-alive, driver threw: "s + e.what());
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
		std::unique_lock guard(pool_->lock_);

		if(pool_->size_ < pool_->min_) {
			try {
				pool_->open_connections(pool_->min_ - pool_->size_);
			} catch(const std::exception& e) { 
				guard.unlock();
				POOL_LOG(Severity::error, "Failed to refill connection pool: "s + e.what());
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
		std::lock_guard guard(pool_->lock_);
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
	PoolManager(ConnectionPool pool, sc::seconds interval, sc::seconds max_idle)
		: pool_(pool),
		  interval_(interval),
		  max_idle_(max_idle) {}

	void check_exceptions() {
		std::lock_guard lock(exception_lock_);

		if(exception_) {
			std::rethrow_exception(exception_);
		}
	}

	void run() try {
		pool_->driver_.thread_enter();
		std::unique_lock lock(cond_lock_);

		while(!stop_) {
			cond_.wait_for(lock, interval_);
			manage_pool();
		}
	} catch(...) {
		std::lock_guard lock(exception_lock_);
		exception_ = std::current_exception();

		POOL_LOG(Severity::debug, "Pool manager trapped exception - passing to next caller");
		pool_->driver_.thread_exit();
	}

	void stop() {
		if(manager_.joinable()) {
			stop_ = true;
			cond_.notify_one();
			manager_.join();
		}

		POOL_LOG(Severity::debug, "Pool manager terminated");
	}

	void start() {
		manager_ = std::thread(&PoolManager::run, this);
		thread::set_name(manager_, "Pool Manager");
	}
};

} // connection_pool, ember

#undef POOL_LOG