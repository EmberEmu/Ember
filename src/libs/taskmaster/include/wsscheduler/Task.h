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

namespace ember { namespace task { namespace ws {

BOOST_STRONG_TYPEDEF(int, TaskID);

struct Task {
	TaskID id;
	TaskID parent;
	TaskID dependency;
	int affinity;
	int open_items;
	int priority;
};

}}} // ws, task, ember