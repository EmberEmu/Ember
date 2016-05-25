/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <wsscheduler/Dequeue.h>
#include <thread>

namespace ember { namespace task { namespace ws {

class Worker {
	std::thread thread_;
	Dequeue work_queue;

	void next_task();

public:
	Worker();

	void add_work();
	void steal_work();
};


}}} // ws, task, ember