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
#include <array>

namespace ember { namespace task { namespace ws {

const int MAX_CONTINUATIONS = 16;
const int CACHELINE_SIZE = 64;

struct alignas(CACHELINE_SIZE) Task {
	Task* parent;
	TaskFunc execute;
	void* args;
	std::atomic<int> counter;
	std::atomic<int> continuation_count;
	std::array<Task*, MAX_CONTINUATIONS> continuations;
};

}}} // ws, task, emberAh,