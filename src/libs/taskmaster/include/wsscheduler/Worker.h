/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <wsscheduler/Dequeue.h>
#include <wsscheduler/Task.h>
#include <shared/threading/Semaphore.h>
#include <shared/threading/Spinlock.h>
#include <logger/Logging.h>
#include <thread>

namespace ember { namespace task { namespace ws {

class Scheduler;

class Worker {
	Scheduler& scheduler_;
	std::thread thread_;
	Dequeue<Task> work_queue;
	log::Logger* logger_;
	
	void run();
	void next_task();
	void run_tasks();

public:
	Worker(Scheduler& scheduler, log::Logger* logger);
	~Worker();

	void add_work();
	void steal_work();
	void stop();

	Worker(Worker&& rhs) : scheduler_(rhs.scheduler_) {
		thread_ = std::move(rhs.thread_);
		//work_queue = std::move(rhs.work_queue);
		logger_ = rhs.logger_;
	}

	friend class Scheduler;
};


}}} // ws, task, ember