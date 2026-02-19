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
	ipv4 = 1,
	ipv6 = 2
};

enum class MessageType : std::uint16_t {
	binding_request              = 0x0001,
	binding_response             = 0x0101,
	binding_error_response       = 0x0111,
	shared_secret_request        = 0x0002,
	shared_secret_response       = 0x0102,
	shared_secret_error_response = 0x0112
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
	mapped_address           = 0x0001,
	response_address         = 0x0002, // rfc3489 only
	change_request           = 0x0003, // rfc3489 & rfc5780 (reinstated)
	source_address           = 0x0004, // rfc3489 only
	changed_address          = 0x0005, // rfc3489 only
	username                 = 0x0006,
	password                 = 0x0007, // rfc3489 only
	message_integrity        = 0x0008,
	error_code               = 0x0009,
	unknown_attributes       = 0x000a,
	reflected_from           = 0x000b, // rfc3489 only
	realm                    = 0x0014,
	nonce                    = 0x0015,
	xor_mapped_address       = 0x0020,
	priority                 = 0x0024, // rfc8445
	use_candidate            = 0x0025, // rfc8445
	padding                  = 0x0026, // rfc5780
	response_port            = 0x0027, // rfc5780
	message_integrity_sha256 = 0x001c, // rfc8489
	password_algorithm       = 0x001d, // rfc8489
	userhash                 = 0x001e, // rfc8489

	// Comprehension optional
	password_algorithms = 0x8002, // rfc8489
	alternate_domain    = 0x8003, // rfc8489
	xor_mapped_addr_opt = 0x8020, // rfc3489 *draft*
	software            = 0x8022,
	alternate_server    = 0x8023,
	cache_timeout       = 0x8027, // rfc5780
	fingerprint         = 0x8028,
	ice_controlled      = 0x8029, // rfc8445
	ice_controlling     = 0x802a, // rfc8445
	response_origin     = 0x802B, // rfc5780
	other_address       = 0x802C  // rfc5780 ("OTHER-ADDRESS uses the same attribute number as CHANGED-ADDRESS", RFC error?)
};

enum class Errors {
	try_alternate     = 300,
	bad_request       = 400,
	unauthorised      = 401,
	unknown_attribute = 420,
	stale_nonce       = 438,
	role_conflict     = 487, // rfc8445
	server_error      = 500
};

enum RFCMode {
	rfc3489,
	rfc5389,
	rfc5780,
	rfc8445
};

smart_enum_class(Hairpinning, std::uint8_t,
	supported,
	not_supported
);

smart_enum_class(Behaviour, std::uint8_t,
	endpoint_independent,
	address_dependent,
	address_port_dependent
);

using AttrReqBy = std::unordered_map<Attributes, std::vector<RFCMode>>;
using AttrValidIn = std::unordered_map<Attributes, MessageType>;
extern AttrReqBy attr_req_lut;
extern AttrValidIn attr_valid_lut;

} // stun, ember