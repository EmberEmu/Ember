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
	log::Logger* logger_;


public:
	//Worker(int index);
	//~Worker();

	//void add_work();
	//void steal_work();
	//void stop();

	friend class Scheduler;
};


}}} // ws, task, ember