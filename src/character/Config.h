/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember {

struct Config {
	bool defer_zone_placement;
	unsigned int max_chars_slots_account;
	unsigned int max_chars_slots_server;
};

} // ember