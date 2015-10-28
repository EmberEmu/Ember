/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember { namespace protocol {

//The game can't handle any other values
const std::uint8_t PRIME_LENGTH = 32;
const std::uint8_t PUB_KEY_LENGTH = 32;

}} //protocol, ember