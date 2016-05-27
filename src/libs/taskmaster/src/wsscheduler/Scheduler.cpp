/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <wsscheduler/Scheduler.h>
#include <shared/util/xoroshiro128plus.h>

namespace ember { namespace task { namespace ws {

thread_local int Scheduler::worker_id_;

Scheduler::Scheduler(std::size_t workers, log::Logger* logger)
                     : WORKER_COUNT_(workers + 1), queues_(workers + 1),
                       stopped_(false), logger_(logger) {
	worker_id_ = 0; // main thread's ID

	for(std::size_t i = 0; i < workers; ++i) {
		workers_.emplace_back(&Scheduler::spawn_worker, this, i + 1);
	}
}

Scheduler::~Scheduler() {
	stop();
}

void Scheduler::stop() {
	stopped_ = true;

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
	while(!stopped_) {
		Task* task = fetch_task();

		if(task) {
			execute(task);
		} else {
			std::this_thread::yield();
		}
	}
}

Task* Scheduler::fetch_task() {
	Dequeue* queue = local_queue();
	Task* task = queue->try_pop_back();

	if(task) {
		return task;
	}

	// local queue empty, try to steal some work
	auto victim_id = rng::xorshift::next() % WORKER_COUNT_;

	for(std::size_t i = 0; i < WORKER_COUNT_; ++i) {
		++victim_id %= WORKER_COUNT_;

		// might be faster to remove this TLS access
		if(victim_id == worker_id_) {
			continue;
		}

		Task* task = queues_[victim_id].try_steal();

		if(task) {
			return task;
		}
	}

	return nullptr;
}

Task* Scheduler::create_task(TaskFunc func, Task* parent) {
	auto task = new Task(); // todo
	task->parent = parent;
	task->execute = func;
	task->counter = 1;

	if(parent) {
		++parent->counter;
	}

	return task;
}

Dequeue* Scheduler::local_queue() {
	return &queues_[worker_id_];
}

void Scheduler::run(Task* task) {
	auto queue = local_queue();
	queue->push_back(task);
}

bool Scheduler::completion_check(Task* task) {
	return task->counter? false : true; // shut msvc up
}

void Scheduler::wait(Task* task) {
	while(task->counter != 0) {
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
	--task->counter;
}
 
}}} // ws, task, ember