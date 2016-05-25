/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/threading/Spinlock.h>
#include <atomic>
#include <cstddef>

namespace ember { namespace task { namespace ws {

class Dequeue {
	Spinlock lock_;
	std::atomic<std::size_t> size_;

public:
	void try_pop_front();
	void try_pop_back();

	void push_back(/* Work work*/);

	std::size_t size();
};

}}} // ws, task, ember