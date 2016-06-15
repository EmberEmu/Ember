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

thread_local int Scheduler::worker_id_ = 0;
thread_local std::size_t Scheduler::allocated_tasks_ = 0;

Scheduler::Scheduler(std::size_t workers, std::size_t max_tasks, log::Logger* logger)
                     : WORKER_COUNT_(workers + 1), MAX_TASKS_(max_tasks), queues_(workers + 1),
                       task_pool_(workers + 1), stopped_(false), logger_(logger) {
	worker_id_ = 0; // main thread's ID

	// allocate a pool of tasks for each worker
	for(auto& pool : task_pool_) {
		pool = std::move(std::vector<Task>(MAX_TASKS_));
	}

	// start the workers
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
			//std::this_thread::yield();
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
		// might be faster to remove this TLS access
		if(victim_id == worker_id_) {
			continue;
		}

		Task* task = queues_[victim_id].try_steal();

		if(task) {
			return task;
		}

		++victim_id %= WORKER_COUNT_;
	}

	return nullptr;
}

Task* Scheduler::create_task(TaskFunc func, Task* parent) {
	auto task = &task_pool_[worker_id_][allocated_tasks_++ % (MAX_TASKS_)];
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
	task->execute(*this, task, task->args);
	finish(task);
}

void Scheduler::finish(Task* task) {
	--task->counter;

	if(task->counter == 0 && task->parent) {
		finish(task->parent);
	}

	// run any continuations
	for(int i = 0; i < task->continuation_count; ++i) {
		run(task->continuations[i]);
	}
}

void Scheduler::add_continuation(Task* ancestor, Task* continuation) {
	auto count = ++ancestor->continuation_count;
	ancestor->continuations[count - 1] = continuation;
}
 
}}} // ws, task, ember