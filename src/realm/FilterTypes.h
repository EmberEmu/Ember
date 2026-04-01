/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/FilterTypes.h>

namespace ember::realm {

// Service specific filters
enum ExtendedFilterType {
	lf_packet_log   = 0x80,    // per-player toggled packet logging
	lf_packet_trace = 0x100    // packet trace log messages
};

} // realm, ember