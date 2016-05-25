/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <wsscheduler/Dequeue.h>
#include <mutex>

namespace ember { namespace task { namespace ws {

void Dequeue::try_pop_front() {
	std::lock_guard<Spinlock> guard(lock_);
}

void Dequeue::try_pop_back() {
	std::lock_guard<Spinlock> guard(lock_);
}

void Dequeue::push_back() {
	std::lock_guard<Spinlock> guard(lock_);
}

std::size_t Dequeue::size() {
	return size_;
}

}}} // ws, task, ember