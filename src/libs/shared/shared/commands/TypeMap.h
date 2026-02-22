/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ArgumentType.h"
#include <typeindex>
#include <boost/unordered/unordered_flat_map.hpp>

namespace ember::commands {

extern boost::unordered_flat_map<ArgumentType, std::type_index> types;

} // commands, ember