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
#include <logger/Logging.h>
#include <vector>
#include <cstddef>

namespace ember { namespace task { namespace ws {

class Worker;

class Scheduler {
	std::vector<Worker> workers_;
	log::Logger* logger_;
	bool stopped_ = false;

public:
	Scheduler(std::size_t workers, log::Logger* logger);
	~Scheduler();

	void run_job(Task task);
	void run_jobs(Task* tasks, std::size_t count, Counter& counter);
	void steal_work(std::size_t victim);
	void stop();
};

}}} // ws, task, ember