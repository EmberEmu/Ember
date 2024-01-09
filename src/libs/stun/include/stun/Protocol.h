/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>
#include <cstdint>
#include <boost/endian/arithmetic.hpp>

/* 
                STUN header as defined by RFC5389

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |0 0|     STUN Message Type     |         Message Length        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Magic Cookie                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                     Transaction ID (96 bits)                  |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

namespace ember::stun {

namespace be = boost::endian;

constexpr std::uint8_t HEADER_LENGTH = 20;
constexpr std::uint8_t ATTR_HEADER_LENGTH = 4;

constexpr std::uint32_t MAGIC_COOKIE = 0x2112A442;

enum class AddressFamily : std::uint8_t {
	IPV4 = 1, IPV6 = 2
};

enum class MessageType : std::uint16_t {
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

// rfc5389 attributes spec
enum class Attributes : std::uint16_t {
	// Comprehension required
	MAPPED_ADDRESS     = 0x0001,
	RESPONSE_ADDRESS   = 0x0002, // rfc3489
	CHANGE_REQUEST     = 0x0003, // rfc3489
	SOURCE_ADDRESS     = 0x0004, // rfc3489
	CHANGED_ADDRESS    = 0x0005, // rfc3489
	USERNAME           = 0x0006,
	PASSWORD           = 0x0007, // rfc3489
	MESSAGE_INTEGRITY  = 0x0008,
	ERROR_CODE         = 0x0009,
	UNKNOWN_ATTRIBUTES = 0x000a,
	REFLECTED_FROM     = 0x000b, // rfc3489
	REALM              = 0x0014,
	NONCE              = 0x0015,
	XOR_MAPPED_ADDRESS = 0x0020,

	// Comprehension optional
	SOFTWARE           = 0x8022,
	ALTERNATE_SERVER   = 0x8023,
	FINGERPRINT        = 0x8028
};

enum class Errors {
	TRY_ALTERNATE     = 300,
	BAD_REQUEST       = 400,
	UNAUTHORISED      = 401,
	UNKNOWN_ATTRIBUTE = 420,
	STALE_NONCE       = 438,
	SERVER_ERROR      = 500
};

} // stun, ember