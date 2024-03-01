/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <cstdint>
#include <shared/smartenum.hpp>

namespace ember::ports {

constexpr std::uint8_t NATPMP_VERSION = 0u;
constexpr std::uint8_t PCP_VERSION = 2u;

constexpr std::uint16_t PORT_OUT = 5351;
constexpr std::uint16_t PORT_IN = 5350;

namespace pcp {

constexpr auto HEADER_SIZE = 24u;

// defined by IANA
enum class Protocol : std::uint8_t {
	ALL = 0x00,
	TCP = 0x06,
	UDP = 0x11
};

enum class Opcode : std::uint8_t {
	ANNOUNCE = 0x00,
	MAP      = 0x01,
	PEER     = 0x02
};

smart_enum_class(Result, std::uint8_t,
	SUCCESS                 = 0x00,
	UNSUPP_VERSION          = 0x01,
	NOT_AUTHORISED          = 0x02,
	MALFORMED_REQUEST       = 0x03,
	UNSUPP_OPCODE           = 0x04,
	UNSUPP_OPTION           = 0x05,
	MALFORMED_OPTION        = 0x06,
	NETWORK_FAILURE         = 0x07,
	NO_RESOURCES            = 0x08,
	UNSUPP_PROTOCOL         = 0x09,
	USER_EX_QUOTA           = 0x0a,
	CANNOT_PROVIDE_EXTERNAL = 0x0b,
	ADDRESS_MISMATCH        = 0x0c,
	EXCESSIVE_REMOTE_PEERS  = 0x0d
);

struct RequestHeader {
	std::uint8_t version;
	bool response;
	Opcode opcode;
	std::uint16_t reserved_0;
	std::uint32_t lifetime;
	std::array<std::uint8_t, 16> client_ip;
};

struct ResponseHeader {
	std::uint8_t version;
	bool response;
	Opcode opcode;
	std::uint8_t reserved_0;
	Result result;
	std::uint32_t lifetime;
	std::uint32_t epoch_time;
	std::array<std::uint8_t, 12> reserved_1;
	std::array<std::uint8_t, 8> data;
	std::uint32_t options;
};

struct OptionHeader {
	std::uint8_t code;
	std::uint8_t reserved;
	std::uint16_t length;
};

struct MapRequest {
	std::array<std::uint8_t, 12> nonce;
	Protocol protocol;
	std::array<std::uint8_t, 3> reserved_0;
	std::uint16_t internal_port;
	std::uint16_t suggested_external_port;
	std::array<std::uint8_t, 16> suggested_external_ip;
};

struct MapResponse {
	std::array<std::uint8_t, 12> nonce;
	Protocol protocol;
	std::array<std::uint8_t, 3> reserved;
	std::uint16_t internal_port;
	std::uint16_t assigned_external_port;
	std::array<std::uint8_t, 16> assigned_external_ip;
};

} // pcp

namespace natpmp {

smart_enum_class(Result, std::uint16_t,
	SUCCESS,
	UNSUPPORTED_VERSION,
	NOT_AUTHORISED,
	NETWORK_FAILURE,
	OUT_OF_RESOURCES,
	UNSUPPORTED_OPCODE
);

enum class RequestOpcode : std::uint8_t {
	REQUEST_EXTERNAL = 0x00
};

enum class Opcode : std::uint8_t {
	EXT = 0x00,
	UDP = 0x01,
	TCP = 0x02,
	RESP_EXT = 128 + EXT,
	RESP_TCP = 128 + TCP,
	RESP_UDP = 128 + UDP
};

struct MapRequest {
	std::uint8_t version;
	Opcode opcode;
	std::uint16_t reserved;
	std::uint16_t internal_port;
	std::uint16_t external_port;
	std::uint32_t lifetime;
};

struct MapResponse {
	std::uint8_t version;
	Opcode opcode;
	Result result_code;
	std::uint32_t secs_since_epoch;
	std::uint16_t internal_port;
	std::uint16_t external_port;
	std::uint32_t lifetime;
};

struct ExtAddressRequest {
	std::uint8_t version;
	RequestOpcode opcode;
};

struct ExtAddressResponse {
	std::uint8_t version;
	Opcode opcode;
	Result result_code;
	std::uint32_t secs_since_epoch;
	std::uint32_t external_ip;
};

struct UnsupportedErrorResponse {
	std::uint8_t version;
	Opcode opcode;
	Result result_code;
	std::uint32_t secs_since_epoch;
};

} // natpmp

  // Types below are used by clients to map to both NAT-PMP & PCP requests
enum class Protocol {
	TCP, UDP, BOTH
};

struct MapRequest {
	std::uint8_t version;
	Protocol protocol;
	std::uint16_t reserved;
	std::uint16_t internal_port;
	std::uint16_t external_port;
	std::uint32_t lifetime;
	std::array<std::uint8_t, 12> nonce;
};

} // ports, ember

CREATE_FORMATTER(ember::ports::pcp::Result)
CREATE_FORMATTER(ember::ports::natpmp::Result)