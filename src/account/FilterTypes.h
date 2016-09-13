/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember {

enum FilterType {
	LF_RESERVED = 1, // do not use explicitly
	LF_MONITORING = 2,
	LF_DB_CONN_POOL = 4,
	LF_NETWORK = 8,
	LF_SPARK = 16,
	LF_NAUGHTY_USER = 32
};

} //ember