/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/Worker.h>
#include <iostream>

namespace ember { namespace log {

Worker::~Worker() {
	if(!stop_) {
		stop();
	}
}

void Worker::process_outstanding_sync() {
	std::tuple<SEVERITY, std::vector<char>, Semaphore<std::mutex>*> item;

	while(queue_sync_.try_dequeue(item)) {
		for(auto& s : sinks_) {
			s->write(std::get<0>(item), std::get<1>(item));
		}
		std::get<2>(item)->signal();
	}
}

void Worker::process_outstanding() {
	if(!queue_.try_dequeue_bulk(std::back_inserter(dequeued_), queue_.size_approx())) {
		return;
	}
		
	std::size_t records = dequeued_.size();

	if(records < 5) {
		for(auto& s : sinks_) {
			for(auto& r : dequeued_) {
				s->write(r.first, r.second);
			}
		}
	} else {
		for(auto& s : sinks_) {
			s->batch_write(dequeued_);
		}
	}

	dequeued_.clear();

	if(dequeued_.capacity() > 100 && records < 100) {
		dequeued_.shrink_to_fit();
	}
}

void Worker::run() {
	while(!stop_) {
		sem_.wait();
		process_outstanding();
		process_outstanding_sync();
	}
}

void Worker::start() {
	thread_ = std::thread(std::bind(&Worker::run, this));
}

void Worker::stop() {
	stop_ = true;
	sem_.signal();
	thread_.join();
	process_outstanding();
}

}} //log, ember