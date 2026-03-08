/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <chrono>

namespace ember {

struct Realm;

} // ember

namespace ember::realm {

struct Config {
	Realm& realm;
	unsigned int realm_id;
	unsigned int max_slots;
	std::chrono::seconds auth_timeout;
	std::chrono::seconds char_list_timeout;
};

} // realm, ember