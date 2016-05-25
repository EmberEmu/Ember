/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <wsscheduler/Worker.h>
#include <thread>
#include <iostream>

namespace ember { namespace task { namespace ws {

Worker::~Worker() {
	if(stopped_) {
		return;
	}

	stop();
}

void Worker::run() {
	while(!stopped_) {
		
	}

	LOG_INFO(logger_) << "Bye" << LOG_SYNC;
}

void Worker::start(log::Logger* logger) {
	logger_ = logger;
	thread_ = std::thread(&Worker::run, this);
}

void Worker::stop() {
	stopped_ = true;

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