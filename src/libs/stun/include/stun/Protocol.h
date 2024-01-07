/*
 * Copyright (c) 2023 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <cstdint>
#include <boost/endian/arithmetic.hpp>

namespace ember::stun {

namespace be = boost::endian;

constexpr std::uint16_t MAGIC_COOKIE = 0x2112A442;

enum class MessageType : std::uint16_t
{
	BINDING_REQUEST              = 0x0001,
	BINDING_RESPONSE             = 0x0101,
	BINDING_ERROR_RESPONSE       = 0x0111,
	SHARED_SECRET_REQUEST        = 0x0002,
	SHARED_SECRET_RESPONSE       = 0x0102,
	SHARED_SECRET_ERROR_RESPONSE = 0x0112
};

struct Header {
	be::big_uint16_t type;
	be::big_uint16_t length;
	be::big_uint32_t cookie;
	be::big_uint8_t trans_id[12];
};

} // stun, ember