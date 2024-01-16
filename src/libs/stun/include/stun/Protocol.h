/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/endian/arithmetic.hpp>
#include <array>
#include <string>
#include <unordered_map>
#include <cstdint>

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
constexpr std::uint8_t HEADER_LEN_OFFSET = 2;

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
	union {
		struct {
			be::big_uint32_t cookie;
			std::array<std::uint32_t, 3> tx_id_5389;
		};
		struct {
			std::array<std::uint32_t, 4> tx_id_3489;
		};
	};
};

// rfc5389 attributes spec
enum class Attributes : std::uint16_t {
	// Comprehension required
	MAPPED_ADDRESS      = 0x0001,
	RESPONSE_ADDRESS    = 0x0002, // rfc3489 only
	CHANGE_REQUEST      = 0x0003, // rfc3489 only
	SOURCE_ADDRESS      = 0x0004, // rfc3489 only
	CHANGED_ADDRESS     = 0x0005, // rfc3489 only
	USERNAME            = 0x0006,
	PASSWORD            = 0x0007, // rfc3489 only
	MESSAGE_INTEGRITY   = 0x0008,
	ERROR_CODE          = 0x0009,
	UNKNOWN_ATTRIBUTES  = 0x000a,
	REFLECTED_FROM      = 0x000b, // rfc3489 only
	REALM               = 0x0014,
	NONCE               = 0x0015,
	XOR_MAPPED_ADDRESS  = 0x0020,

	// Comprehension optional
	XOR_MAPPED_ADDR_OPT = 0x8020, // todo, which RFC?
	SOFTWARE            = 0x8022,
	ALTERNATE_SERVER    = 0x8023,
	FINGERPRINT         = 0x8028,
	OTHER_ADDRESS       = 0x802C // todo, which RFC?
};

enum class Errors {
	TRY_ALTERNATE     = 300,
	BAD_REQUEST       = 400,
	UNAUTHORISED      = 401,
	UNKNOWN_ATTRIBUTE = 420,
	STALE_NONCE       = 438,
	SERVER_ERROR      = 500
};

enum RFCMode {
	RFC5389 = 0x01,
	RFC3489 = 0x02,
	RFC_BOTH = RFC5389 | RFC3489
};

using AttrReqBy = std::unordered_map<Attributes, RFCMode>;
extern AttrReqBy attr_req_lut;

} // stun, ember