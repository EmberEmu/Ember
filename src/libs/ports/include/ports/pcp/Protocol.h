/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <ports/Protocol.h>
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
	all = 0x00,
	tcp = 0x06,
	udp = 0x11
};

enum class Opcode : std::uint8_t {
	announce = 0x00,
	map      = 0x01,
	peer     = 0x02
};

smart_enum_class(Result, std::uint8_t,
	success                 = 0x00,
	unsupp_version          = 0x01,
	not_authorised          = 0x02,
	malformed_request       = 0x03,
	unsupp_opcode           = 0x04,
	unsupp_option           = 0x05,
	malformed_option        = 0x06,
	network_failure         = 0x07,
	no_resources            = 0x08,
	unsupp_protocol         = 0x09,
	user_ex_quota           = 0x0a,
	cannot_provide_external = 0x0b,
	address_mismatch        = 0x0c,
	excessive_remote_peers  = 0x0d
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

enum class OptionCode : std::uint8_t {
	third_party    = 0x01,
	prefer_failure = 0x02,
	filter         = 0x03
};

struct OptionHeader {
	OptionCode code;
	std::uint8_t reserved;
	std::uint16_t length;
};

} // pcp

namespace natpmp {

smart_enum_class(Result, std::uint16_t,
	success,
	unsupported_version,
	not_authorised,
	network_failure,
	out_of_resources,
	unsupported_opcode
);

enum class Opcode : std::uint8_t {
	request_external = 0x00,
	udp = 0x01,
	tcp = 0x02,
	resp_ext = 128 + request_external,
	resp_tcp = 128 + tcp,
	resp_udp = 128 + udp
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
	Opcode opcode;
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

struct MapRequest {
	std::uint8_t version;
	Protocol protocol;
	std::uint16_t reserved;
	std::uint16_t internal_port;
	std::uint16_t external_port;
	std::uint32_t lifetime;
	std::uint32_t epoch;
	std::array<std::uint8_t, 12> nonce;
	std::array<std::uint8_t, 16> external_ip;
};

} // ports, ember

CREATE_FORMATTER(ember::ports::pcp::Result)
CREATE_FORMATTER(ember::ports::natpmp::Result)