/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/ArgumentValue.h>
#include <boost/unordered/unordered_flat_map.hpp>
#include <string>
#include <unordered_map>
#include <utility>

namespace ember::commands::args {

using Map = boost::unordered_flat_map<std::string, args::Value>;

} // args, commands, ember