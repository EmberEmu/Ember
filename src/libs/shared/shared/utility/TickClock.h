/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember::utility {

/*
 * This is ideal for when we need a very fast time source for a task and we're fine
 * that the timer isn't that precise (e.g. events measured in the seconds).
 */
std::uint64_t get_tick_count();

}; // utility, ember