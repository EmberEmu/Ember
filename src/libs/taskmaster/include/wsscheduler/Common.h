/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/serialization/strong_typedef.hpp>
#include <atomic>
#include <memory>


namespace ember { namespace task { namespace ws {

class Scheduler;

#define TASK_ENTRY_POINT(func_name) \
	void func_name(ts::Scheduler& scheduler, void* arg)

typedef void (*TaskFunc)(Scheduler&, void*);
typedef std::atomic<int> Counter;

}}} // ws, task, ember