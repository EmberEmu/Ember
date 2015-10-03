/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember { namespace filter {

enum LogFilter : std::uint32_t {
	LOG_RESERVED_    = 1,
	LOG_INIT         = 2,
	LOG_NETWORK      = 4
};

}} // filter, ember