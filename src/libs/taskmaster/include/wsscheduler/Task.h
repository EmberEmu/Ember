/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <wsscheduler/Common.h>
#include <atomic>

namespace ember { namespace task { namespace ws {

struct Task {
	TaskPtr execute;
	void* arg;
	//std::atomic<int> open_count;
};

typedef std::unique_ptr<Task> TaskHandle

}}} // ws, task, ember