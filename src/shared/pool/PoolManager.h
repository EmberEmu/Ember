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

namespace ember { namespace connection_pool {

template<typename ConType, typename Driver, typename ReusePolicy, typename GrowthPolicy>
class PoolManager {
	typedef Pool<Driver, ReusePolicy, GrowthPolicy>* ConnectionPool;
	ConnectionPool pool_;
	unsigned int interval_ = 1;
	unsigned int max_idle_ = 5;
	std::thread manager_;
	bool stop_ = false;
	Spinlock exception_lock_;
	std::exception_ptr exception_;

	void close_excess_idle(std::vector<ConType>& connections) {
		for(auto& c : connections) {
			try {
				pool_->driver_.close(c);
			} catch (...) {}
		}
	}

	void refresh_idle(std::vector<ConnDetail<ConType>*>& connections) {
		for(auto& c : connections) {
			try {
				c->idle = 0;
				c->conn = pool_->driver_.keep_alive(c->conn);
				c->error = false;
			} catch (...) {
				c->error = true;
			}

			c->checked_out = false;
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
				std::cout << i->idle << " but max idle is " << max_idle_ << std::endl;
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

		unsigned int last_check = 0;
		const std::chrono::seconds sleep_time(1);

		while(!stop_) {
			std::this_thread::sleep_for(sleep_time);
			last_check += 1;

			if(last_check >= interval_) {
				last_check = 0;
				manage_connections();
			}
		}

		pool_->driver_.thread_exit();
	} catch(...) {
		std::unique_lock<Spinlock>(exception_lock_);
		exception_ = std::current_exception();
	}

	void stop() {
		stop_ = true;
		manager_.join();
	}

	void start(unsigned int interval = 10, unsigned int max_idle = 5) {
		interval_ = interval;
		max_idle_ = max_idle;
		manager_ = std::thread(std::bind(&PoolManager::run, this));
	}
};

}} //connection_pool, ember