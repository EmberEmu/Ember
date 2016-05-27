/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <wsscheduler/Worker.h>
#include <wsscheduler/Scheduler.h>
#include <thread>
#include <iostream>

namespace ember { namespace task { namespace ws {

Worker::Worker(Scheduler& scheduler, log::Logger* logger)
               : logger_(logger), scheduler_(scheduler) {
	thread_ = std::thread(&Worker::run, this);
}

Worker::~Worker() {
	stop();
}

void Worker::run() {
	while(!scheduler_.stopped_) {
		scheduler_.semaphore_.wait(); // wait for a task to be submitted
		run_tasks();
		scheduler_.idle_workers_.signal(); // mark ourselves as idle
	}
}

void Worker::run_tasks() {
	Task task;

	while(work_queue.try_pop_back(task)) {
		task.execute(&scheduler_, task.arg);
	}
	// pop task from back of dequeue
	// dequeue empty? -> steal work
	// all other queues empty? -> switch to idle
}

void Worker::stop() {
	if(thread_.joinable()) {
		thread_.join();
	}
}

void Worker::steal_work() {

}

void Worker::next_task() {

}

void Worker::add_work() {

}

}}} // ws, task, ember