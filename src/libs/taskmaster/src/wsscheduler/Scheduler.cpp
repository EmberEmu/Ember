/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <wsscheduler/Scheduler.h>

namespace ember { namespace task { namespace ws {

thread_local int Scheduler::worker_id_;

Scheduler::Scheduler(std::size_t workers, log::Logger* logger)
                     : queues_(workers), logger_(logger) {
	worker_id_ = 0; // main thread's ID

	for(std::size_t i = 0; i < workers; ++i) {
		workers_.emplace_back(&Scheduler::spawn_worker, this, i + 1);
	}
}

Scheduler::~Scheduler() {
	stop();
}

void Scheduler::stop() {
	for(auto& worker : workers_) {
		if(worker.joinable()) {
			worker.join();
		}
	}
}

void Scheduler::spawn_worker(int index) {
	worker_id_ = index;
	start_worker();
}

void Scheduler::start_worker() {

}

Task* Scheduler::create_task(TaskFunc func, Task* parent) {
	auto task = new Task();
	task->parent = parent;
	task->execute = func;
	task->counter = 1;

	if(parent) {
		++parent->counter;
	}

	return task;
}

Dequeue<Task*>* Scheduler::local_queue() {
	return &queues_[worker_id_];
}

void Scheduler::run(Task* task) {
	auto queue = local_queue();
	queue->push_back(task);
}

bool Scheduler::completion_check(Task* task) {
	return task->counter? true : false; // shut msvc up
}

Task* Scheduler::fetch_task() {
	return nullptr;
}

void Scheduler::wait(Task* task) {
	while(!completion_check(task)) {
		Task* next = fetch_task();

		if(next) {
			execute(next);
		}
	}
}

void Scheduler::execute(Task* task) {
	task->execute(*this, task->args);
	finish(task);
}

void Scheduler::finish(Task* task) {

}
 
}}} // ws, task, ember