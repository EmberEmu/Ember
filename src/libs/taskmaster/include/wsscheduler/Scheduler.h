/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>
#include <cstddef>

namespace ember { namespace task { namespace ws {

class Worker;

class Scheduler {
	std::vector<Worker> workers_;

public:
	explicit Scheduler(std::size_t workers);

	void add_work_begin();
	void add_work_finish();
	void steal_work(std::size_t victim);
};

}}} // ws, task, ember