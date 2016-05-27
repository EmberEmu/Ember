/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <wsscheduler/Worker.h>
#include <wsscheduler/Common.h>
#include <wsscheduler/Task.h>
#include <shared/threading/Semaphore.h>
#include <logger/Logging.h>
#include <atomic>
#include <vector>
#include <cstddef>

namespace ember { namespace task { namespace ws {

class Worker;

class Scheduler {
	std::vector<Worker> workers_;
	Semaphore<Spinlock> semaphore_;
	Semaphore<Spinlock> idle_workers_;
	std::atomic_bool root_completed_ = false;
	std::atomic_bool stopped_ = false;
	log::Logger* logger_;

public:
	Scheduler(std::size_t workers, log::Logger* logger);
	~Scheduler();

	void submit_task(Task task);
	void submit_tasks(Task* tasks, std::size_t count, Counter& counter);
	void steal_work(std::size_t victim);
	void stop();

	friend class Worker;
};

}}} // ws, task, ember