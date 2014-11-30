/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Policies.h"
#include "Connection.h"
#include "ConnectionPool.h"
#include "../Spinlock.h"
#include <vector>
#include <cassert>
#include <thread>
#include <list>
#include <exception>
#include <mutex>
#include <condition_variable>

namespace ember { namespace connection_pool {

namespace sc = std::chrono;

template<typename ConType, typename Driver, typename ReusePolicy, typename GrowthPolicy>
class PoolManager {
	typedef Pool<Driver, ReusePolicy, GrowthPolicy>* ConnectionPool;
	ConnectionPool pool_;
	sc::seconds interval_, max_idle_;
	std::thread manager_;
	bool stop_ = false;
	Spinlock exception_lock_;
	std::exception_ptr exception_;
	std::condition_variable cond_;
	std::mutex cond_lock_;

	void close_excess_idle(std::vector<ConType>& connections) {
		for(auto& c : connections) {
			try {
				pool_->driver_.close(c);
			} catch(...) {}
		}
	}

	void refresh_idle(std::vector<ConnDetail<ConType>*>& connections) {
		for(auto& c : connections) {
			try {
				c->idle = sc::seconds(0);
				c->conn = pool_->driver_.keep_alive(c->conn);
				c->error = false;
			} catch(...) {
				c->error = true;
			}

			c->checked_out = false;
			pool_->semaphore_.signal();
		}
	}

	void close_errored() {
		std::unique_lock<Spinlock> guard(pool_->lock_);

		for(auto i = pool_->pool_.begin(); i != pool_->pool_.end();) {
			if(i->error) {
				i = pool_->pool_.erase(i);
			} else {
				++i; 
			}
		}

		//If the pool has grown too small, try to refill it
		if(pool_->pool_.size() < pool_->min_) {
			pool_->open_connections(pool_->min_ - pool_->pool_.size());
		}
	}

	void manage_connections() {
		std::vector<ConnDetail<ConType>*> checked_out;
		std::vector<ConType> removed;

		std::unique_lock<Spinlock> guard(pool_->lock_);

		int removals = pool_->pool_.size() - pool_->min_;

		for(auto i = pool_->pool_.begin(); i != pool_->pool_.end();) {
			if(i->checked_out) {
				++i;
				continue;
			}

			if(i->idle < max_idle_) {
				i->idle += interval_;
				++i;
			} else if(removals > 0) {
				--removals;
				removed.push_back(i->conn);
				i = pool_->pool_.erase(i);
			} else {
				i->checked_out = true;
				checked_out.push_back(&*i);
				++i;
			}
		}

		pool_->lock_.unlock();

		close_excess_idle(removed);
		refresh_idle(checked_out);
		close_errored();
	}

public:
	PoolManager(ConnectionPool pool) {
		pool_ = pool;
	}

	void check_exceptions() {
		std::unique_lock<Spinlock>(exception_lock_);

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

			manage_connections();
		}

		pool_->driver_.thread_exit();
	} catch(...) {
		std::unique_lock<Spinlock>(exception_lock_);
		exception_ = std::current_exception();
	}

	void stop() {
		if(manager_.joinable()) {
			stop_ = true;
			cond_.notify_one();
			manager_.join();
		}
	}

	void start(sc::seconds interval, sc::seconds max_idle) {
		interval_ = interval;
		max_idle_ = max_idle;
		manager_ = std::thread(std::bind(&PoolManager::run, this));
	}
};

}} //connection_pool, ember