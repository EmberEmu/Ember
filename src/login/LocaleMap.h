/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "grunt/Magic.h"
#include <dbcreader/MemoryDefs.h>
#include <unordered_map>

namespace ember {

extern const std::unordered_map<grunt::Locale, dbc::Cfg_Categories::Region> locale_map;

} // ember