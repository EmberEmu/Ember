/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <wsscheduler/Scheduler.h>

namespace ember { namespace task { namespace ws {

Scheduler::Scheduler(std::size_t workers, log::Logger* logger)
                     : idle_workers_(workers), logger_(logger) {
	for(std::size_t i = 0; i < workers; ++i) {
		workers_.emplace_back(*this, logger);
	}
}

Scheduler::~Scheduler() {
	stop();
}

void Scheduler::stop() {
	for(auto& worker : workers_) {
		worker.stop();
	}
}

void Scheduler::steal_work(std::size_t victim) {

}

void Scheduler::submit_task(Task task) {
	for(auto& worker : workers_) {
		idle_workers_.wait();
	}

	semaphore_.signal_all(workers_.size());
	task.execute(this, task.arg);
}

void Scheduler::submit_tasks(Task* tasks, std::size_t count, Counter& counter) {
	for(std::size_t i = 0; i < count; ++i) {
		tasks[i].execute(this, tasks[i].arg);
	}

	counter = 0;
}

}}} // ws, task, ember