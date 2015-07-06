/*
 * Copyright (c) 2014, 2015 Ember
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
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <exception>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <cstddef>

namespace ember { namespace connection_pool {

namespace sc = std::chrono;

template<typename Driver, typename ReusePolicy, typename GrowthPolicy> class Pool;

template<typename ConType, typename Driver, typename ReusePolicy, typename GrowthPolicy>
class PoolManager {
	typedef Pool<Driver, ReusePolicy, GrowthPolicy>* ConnectionPool;
	ConnectionPool pool_;
	Spinlock exception_lock_;
	sc::seconds interval_, max_idle_;
	std::thread manager_;
	std::exception_ptr exception_;
	std::condition_variable cond_;
	std::mutex cond_lock_;
	bool stop_ = false;

	void close(ConnDetail<ConType>& conn) {
		try {
			pool_->driver_.close(conn.conn);
		} catch(std::exception& e) { 
			if(pool_->log_cb_) {
				pool_->log_cb_(Severity::WARN,
				               std::string("Connection close, driver threw: ") + e.what());
			}
		}
			
		conn.reset();
		--pool_->size_;
	}

	void refresh(ConnDetail<ConType>& conn) {
		try {
			conn.conn = pool_->driver_.keep_alive(conn.conn);
			conn.idle = sc::seconds(0);
			conn.error = false;
		} catch(std::exception& e) { 
			if(pool_->log_cb_) {
				pool_->log_cb_(Severity::WARN,
				               std::string("Connection keep-alive, driver threw: ") + e.what());
			}
			conn.error = true;
		}

		conn.refresh = false;
		conn.checked_out = false;

		if(!conn.error) {
			pool_->semaphore_.signal();
		}
	}

	// If the pool has grown too small, try to refill it
	void refill() {
		std::unique_lock<Spinlock> guard(pool_->lock_);

		if(pool_->size_ < pool_->min_) {
			try {
				pool_->open_connections(pool_->min_ - pool_->size_);
			} catch(std::exception& e) { 
				guard.unlock();

				if(pool_->log_cb_) {
					pool_->log_cb_(Severity::WARN,
					               std::string("Failed to refill connection pool: ") + e.what());
				}
			}
		}
	}

	void maintain_connections() {
		for(auto& conn : pool_->pool_) {
			if(conn.empty_slot) {
				continue;
			} else if(conn.refresh) {
				refresh(conn);
			} else if(conn.sweep || conn.error) {
				close(conn);
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

		while(!stop_) {
			if(cond_.wait_for(lock, interval_) == std::cv_status::no_timeout) {
				break;
			}

			manage_pool();
		}

		pool_->driver_.thread_exit();
	} catch(...) {
		std::lock_guard<Spinlock> lock(exception_lock_);
		exception_ = std::current_exception();

		if(pool_->log_cb_) {
			pool_->log_cb_(Severity::DEBUG,
			               "Pool manager trapped exception - passing to next caller");
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
		manager_ = std::thread(&PoolManager::run, this);
	}
};

}} // connection_pool, ember