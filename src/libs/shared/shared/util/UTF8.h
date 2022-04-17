/*
 * Copyright (c) 2016 - 2022 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "UTF8String.h"
#include <string>
#include <cstddef>

namespace ember::util::utf8 {

std::size_t max_consecutive(const utf8_string& string);
std::size_t length(const utf8_string& utf8_string);
bool is_valid(const utf8_string& utf8_string);
bool is_valid(const char* utf8_string, std::size_t byte_length);

} // utf8, util, ember