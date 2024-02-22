/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/smartenum.hpp>
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
constexpr std::uint8_t FP_ATTR_LENGTH = 8;
constexpr std::uint8_t PADDING_ROUND = 4;
constexpr std::uint8_t RESPONSE_PORT_BODY_LEN = 4;
constexpr std::uint8_t CHANGE_REQUEST_BODY_LEN = 4;
constexpr std::uint8_t FINGERPRINT_BODY_LEN = 4;
constexpr std::uint8_t RESPONSE_PORT_LEN = 4;

constexpr std::uint8_t CHANGE_IP_MASK = 0x01 << 2;
constexpr std::uint8_t CHANGE_PORT_MASK = 0x01 << 1;

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

union TxID {
	struct {
		std::array<std::uint32_t, 3> id_5389;
	};
	struct {
		std::array<std::uint32_t, 4> id_3489;
	};
};

struct Header {
	be::big_uint16_t type;
	be::big_uint16_t length;
	be::big_uint32_t cookie;
	TxID tx_id;
};

// rfc5389 attributes spec
enum class Attributes : std::uint16_t {
	// Comprehension required
	MAPPED_ADDRESS           = 0x0001,
	RESPONSE_ADDRESS         = 0x0002, // rfc3489 only
	CHANGE_REQUEST           = 0x0003, // rfc3489 & rfc5780 (reinstated)
	SOURCE_ADDRESS           = 0x0004, // rfc3489 only
	CHANGED_ADDRESS          = 0x0005, // rfc3489 only
	USERNAME                 = 0x0006,
	PASSWORD                 = 0x0007, // rfc3489 only
	MESSAGE_INTEGRITY        = 0x0008,
	ERROR_CODE               = 0x0009,
	UNKNOWN_ATTRIBUTES       = 0x000a,
	REFLECTED_FROM           = 0x000b, // rfc3489 only
	REALM                    = 0x0014,
	NONCE                    = 0x0015,
	XOR_MAPPED_ADDRESS       = 0x0020,
	PRIORITY                 = 0x0024, // rfc8445
	USE_CANDIDATE            = 0x0025, // rfc8445
	PADDING                  = 0x0026, // rfc5780
	RESPONSE_PORT            = 0x0027, // rfc5780
	MESSAGE_INTEGRITY_SHA256 = 0x001c, // rfc8489
	PASSWORD_ALGORITHM       = 0x001d, // rfc8489
	USERHASH                 = 0x001e, // rfc8489

	// Comprehension optional
	PASSWORD_ALGORITHMS = 0x8002, // rfc8489
	ALTERNATE_DOMAIN    = 0x8003, // rfc8489
	XOR_MAPPED_ADDR_OPT = 0x8020, // rfc3489 *draft*
	SOFTWARE            = 0x8022,
	ALTERNATE_SERVER    = 0x8023,
	CACHE_TIMEOUT       = 0x8027, // rfc5780
	FINGERPRINT         = 0x8028,
	ICE_CONTROLLED      = 0x8029, // rfc8445
	ICE_CONTROLLING     = 0x802a, // rfc8445
	RESPONSE_ORIGIN     = 0x802B, // rfc5780
	OTHER_ADDRESS       = 0x802C  // rfc5780 ("OTHER-ADDRESS uses the same attribute number as CHANGED-ADDRESS", RFC error?)
};

enum class Errors {
	TRY_ALTERNATE     = 300,
	BAD_REQUEST       = 400,
	UNAUTHORISED      = 401,
	UNKNOWN_ATTRIBUTE = 420,
	STALE_NONCE       = 438,
	ROLE_CONFLICT     = 487, // rfc8445
	SERVER_ERROR      = 500
};

enum RFCMode {
	RFC3489,
	RFC5389,
	RFC5780,
	RFC8445
};

smart_enum_class(Hairpinning, std::uint8_t,
	SUPPORTED, NOT_SUPPORTED
);

smart_enum_class(Behaviour, std::uint8_t,
	ENDPOINT_INDEPENDENT,
	ADDRESS_DEPENDENT,
	ADDRESS_PORT_DEPENDENT
);

using AttrReqBy = std::unordered_map<Attributes, std::vector<RFCMode>>;
using AttrValidIn = std::unordered_map<Attributes, MessageType>;
extern AttrReqBy attr_req_lut;
extern AttrValidIn attr_valid_lut;

} // stun, ember