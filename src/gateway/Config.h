/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/Realm.h>
#include <chrono>
#include <cstddef>

namespace ember::gateway {

struct Config {
	Realm& realm;
	bool list_zone_hide;
	unsigned int max_slots;
	std::chrono::seconds auth_timeout;
};

} // gateway, ember