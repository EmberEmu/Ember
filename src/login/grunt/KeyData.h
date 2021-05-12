/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/endian/arithmetic.hpp>
#include <array>
#include <cstdint>

namespace ember::grunt {

namespace be = boost::endian;

// These are all 50/50 guesses, take with a grain of salt
struct KeyData {
	be::little_uint16_t len;
	be::little_uint32_t pub_value;
	std::array<std::uint8_t, 4> product;
	std::array<std::uint8_t, 20> hash; // login proof: hashed with 'A' or 'salt' if reconnect proof
};

} // grunt, ember