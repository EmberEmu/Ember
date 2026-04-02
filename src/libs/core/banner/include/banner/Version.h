/*
 * Copyright (c) 2014 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once 

#include <string_view>

namespace ember::build {

extern std::string_view git_hash;
extern std::string_view version;
extern std::string_view date;
extern std::string_view time;

} // build, ember