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
#include <vector>
#include <cassert>
#include <thread>

namespace ConnectionPool {

class ConnectionPool;

template<typename ConType>
class PoolManager {
	ConnectionPool* pool;
	unsigned int interval;
	unsigned int max_idle;
	std::thread manager;

	void close_excess_idle(std::vector<ConType>& connections) {
		for(auto& c : connections) {
			try {
				driver.close(c);
			} catch (...) {}
		}
	}

	void refresh_idle(std::vector<ConnDetail<ConType>*>& connections) {
		for(auto& c : connections) {
			try {
				c->idle = 0;
				c->conn = driver.keep_alive(c->conn);
				c->error = false;
			} catch (...) {
				c->error = true;
			}

			c->checked_out = false;
		}
	}

	void close_errored() {
		std::unique_lock<Spinlock> guard(pool.lock);

		for(auto i = pool.begin(); i != pool.end();) {
			if(i->error) {
				i = pool.erase();
			}
		}
	}

	void manage_connections() {
		std::vector<ConnDetail<ConType>*> checked_out;
		std::vector<ConType> removed;

		std::unique_lock<Spinlock> guard(lock);
		int removals = pool.size() - min;

		for(auto i = pool.begin(); i != pool.end();) {
			if(i->idle < max_idle) {
				i->idle += interval;
				++i;
			} else if(removals > 0) {
				--removals;
				removed.push_back(i->conn);
				i = pool.erase(i);
			} else {
				i->checked_out = true;
				checked_out.push_back(&*i);
				++i;
			}
		}

		lock.unlock();

		close_excess_idle(removed);
		refresh_idle(checked_out);
		//remove_errored();
	}

public:
	PoolManager(ConnectionPool* pool) : pool(pool) {}

	void run() try {
		driver.thread_enter();

		unsigned int last_check = 0;
		const std::chrono::seconds sleep_time(1);

		while(!stop) {
			std::this_thread::sleep_for(sleep_time);
			last_check += 1;

			if(last_check >= interval) {
				last_check = 0;
				manage_connections();
			}
		}

		driver.thread_exit();
	} catch(...) {
		std::cout << "WHAT?";
	}

	void stop() {
		stop = true;
		manager.join();
	}

	void start(unsigned int interval = 10, unsigned int max_idle = 300) {
		this->interval = interval;
		this->max_idle = max_idle;
		manager = std::thread(&Pool::run, this);
	}
};

} //ConnectionPool