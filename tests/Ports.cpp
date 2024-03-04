/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PortsVectors.h"
#include <ports/pcp/Serialise.h>
#include <ports/pcp/Deserialise.h>
#include <boost/endian.hpp>
#include <gtest/gtest.h>
#include <boost/asio/ip/address.hpp>
#include <array>
#include <string>
#include <string_view>
#include <cstdint>

using namespace ember;
using namespace ember::ports;

class Ports : public ::testing::Test {
public:
	Ports() : stream(adaptor) {}

	const std::array<std::uint8_t, 16> ip {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11
	};

	const std::array<std::uint8_t, 12> nonce {
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06
	};

	std::vector<std::uint8_t> buffer;
	spark::v2::BufferAdaptor<std::vector<std::uint8_t>> adaptor = buffer;
	spark::v2::BinaryStream<spark::v2::BufferAdaptor<std::vector<std::uint8_t>>> stream;
};

TEST_F(Ports, PCP_RequestHeaderRoundtrip) {
	const pcp::RequestHeader input {
		.version = PCP_VERSION,
		.response = true,
		.opcode = pcp::Opcode::MAP,
		.reserved_0 = 0xF00D,
		.lifetime = 7200u,
	};

	ports::serialise(input, stream);
	const auto output = deserialise<pcp::RequestHeader>(buffer);
	ASSERT_EQ(input.client_ip, output.client_ip);
	ASSERT_EQ(input.lifetime, output.lifetime);
	ASSERT_EQ(input.opcode, output.opcode);
	ASSERT_EQ(input.reserved_0, output.reserved_0);
	ASSERT_EQ(input.response, output.response);
	ASSERT_EQ(input.version, output.version);
}

TEST_F(Ports, PCP_ResponseHeaderRoundtrip) {
	const std::array<std::uint8_t, 12> reserved_1 {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	};

	const pcp::ResponseHeader input {
		.version = PCP_VERSION,
		.response = true,
		.opcode = pcp::Opcode::MAP,
		.reserved_0 = 0xFF,
		.result = pcp::Result::ADDRESS_MISMATCH,
		.lifetime = 0xAABBCCDD,
		.epoch_time = 0x01020304,
		.reserved_1 = reserved_1
	};

	ports::serialise(input, stream);
	const auto output = deserialise<pcp::ResponseHeader>(buffer);
	ASSERT_EQ(input.version, output.version);
	ASSERT_EQ(input.response, output.response);
	ASSERT_EQ(input.opcode, output.opcode);
	ASSERT_EQ(input.reserved_0, output.reserved_0);
	ASSERT_EQ(input.result, output.result);
	ASSERT_EQ(input.lifetime, output.lifetime);
	ASSERT_EQ(input.epoch_time, output.epoch_time);
	ASSERT_EQ(input.reserved_1, output.reserved_1);
}

TEST_F(Ports, PCP_MapResponseRoundtrip) {
	const pcp::MapResponse input {
		.protocol = pcp::Protocol::TCP,
		.reserved = { 0x01, 0x02, 0x03 },
		.internal_port = 0xCAFE,
		.assigned_external_port = 0xBEEF,
		.assigned_external_ip = ip
	};

	ports::serialise(input, stream);
	const auto output = deserialise<pcp::MapResponse>(buffer);
	ASSERT_EQ(input.protocol, output.protocol);
	ASSERT_EQ(input.reserved, output.reserved);
	ASSERT_EQ(input.internal_port, output.internal_port);
	ASSERT_EQ(input.assigned_external_ip, output.assigned_external_ip);
	ASSERT_EQ(input.assigned_external_port, output.assigned_external_port);
}

TEST_F(Ports, PCP_MapRequestRoundtrip) {
	const pcp::MapRequest input {
		.nonce = nonce,
		.protocol = pcp::Protocol::TCP,
		.reserved_0 = { 0x04, 0x05, 0x06 },
		.internal_port = 0xBEEF,
		.suggested_external_port = 0xCAFE,
		.suggested_external_ip = ip
	};

	ports::serialise(input, stream);
	const auto output = deserialise<pcp::MapRequest>(buffer);
	ASSERT_EQ(input.nonce, output.nonce);
	ASSERT_EQ(input.protocol, output.protocol);
	ASSERT_EQ(input.reserved_0, output.reserved_0);
	ASSERT_EQ(input.internal_port, output.internal_port);
	ASSERT_EQ(input.suggested_external_port, input.suggested_external_port);
	ASSERT_EQ(input.suggested_external_ip, input.suggested_external_ip);
}

TEST_F(Ports, PCP_OptionHeaderRoundtrip) {
	const pcp::OptionHeader input {
		.code = pcp::OptionCode::PREFER_FAILURE,
		.reserved = 0xFE,
		.length = 0xFAFA
	};

	ports::serialise(input, stream);
	const auto output = deserialise<pcp::OptionHeader>(buffer);
	ASSERT_EQ(input.code, output.code);
	ASSERT_EQ(input.reserved, output.reserved);
	ASSERT_EQ(input.length, output.length);
}

TEST_F(Ports, NATPMP_MapRequestRoundtrip) {
	const natpmp::MapRequest input {
		.version = 0,
		.opcode = natpmp::Opcode::RESP_TCP,
		.reserved = 0xFFFF,
		.internal_port = 0xCAFE,
		.external_port = 0xBEEF,
		.lifetime = 0xABBCCDDE
	};

	ports::serialise(input, stream);
	const auto output = deserialise<natpmp::MapRequest>(buffer);
	ASSERT_EQ(input.version, output.version);
	ASSERT_EQ(input.opcode, output.opcode);
	ASSERT_EQ(input.reserved, output.reserved);
	ASSERT_EQ(input.internal_port, output.internal_port);
	ASSERT_EQ(input.external_port, output.external_port);
	ASSERT_EQ(input.lifetime, output.lifetime);
}

TEST_F(Ports, NATPMP_MapResponseRoundtrip) {
	const natpmp::MapResponse input {
		.version = 0,
		.opcode = natpmp::Opcode::TCP,
		.result_code = natpmp::Result::UNSUPPORTED_OPCODE,
		.secs_since_epoch = 0xBAADF00D,
		.internal_port = 0xBEEF,
		.external_port = 0xCAFE,
		.lifetime = 0xABBCCDDE
	};

	ports::serialise(input, stream);
	const auto output = deserialise<natpmp::MapResponse>(buffer);
	ASSERT_EQ(input.version, output.version);
	ASSERT_EQ(input.opcode, output.opcode);
	ASSERT_EQ(input.result_code, output.result_code);
	ASSERT_EQ(input.internal_port, output.internal_port);
	ASSERT_EQ(input.external_port, output.external_port);
	ASSERT_EQ(input.lifetime, output.lifetime);
}

TEST_F(Ports, NATPMP_ExtAddressRequestRoundtrip) {
	const natpmp::ExtAddressRequest input {
		.version = 0,
		.opcode = natpmp::Opcode::REQUEST_EXTERNAL
	};

	ports::serialise(input, stream);
	const auto output = deserialise<natpmp::ExtAddressRequest>(buffer);
	ASSERT_EQ(input.version, output.version);
	ASSERT_EQ(input.opcode, output.opcode);
}

TEST_F(Ports, NATPMP_ExtAddressResponseRoundtrip) {
	const natpmp::ExtAddressResponse input {
		.version = 0,
		.opcode = natpmp::Opcode::RESP_EXT,
		.result_code = natpmp::Result::SUCCESS,
		.secs_since_epoch = 0xBAADF00D,
		.external_ip = 0xC0A80001
	};

	ports::serialise(input, stream);
	const auto output = deserialise<natpmp::ExtAddressResponse>(buffer);
	ASSERT_EQ(input.version, output.version);
	ASSERT_EQ(input.opcode, output.opcode);
	ASSERT_EQ(input.result_code, output.result_code);
	ASSERT_EQ(input.secs_since_epoch, output.secs_since_epoch);
	ASSERT_EQ(input.external_ip, output.external_ip);
}

TEST_F(Ports, NATPMP_UnsupportedErrorResponseRoundtrip) {
	const natpmp::UnsupportedErrorResponse input {
		.version = 0,
		.opcode = natpmp::Opcode::RESP_EXT,
		.result_code = natpmp::Result::UNSUPPORTED_VERSION,
		.secs_since_epoch = 0xBAADF00D
	};

	ports::serialise(input, stream);
	const auto output = deserialise<natpmp::UnsupportedErrorResponse>(buffer);
	ASSERT_EQ(input.version, output.version);
	ASSERT_EQ(input.opcode, output.opcode);
	ASSERT_EQ(input.result_code, output.result_code);
	ASSERT_EQ(input.secs_since_epoch, output.secs_since_epoch);
}

TEST_F(Ports, PCP_TestRequestVector) {
	const auto address = boost::asio::ip::address::from_string("::ffff:192.168.0.4");
	const auto v6_bytes = address.to_v6().to_bytes();

	const auto request = deserialise<pcp::RequestHeader>(pcp_mapping_request);
	ASSERT_EQ(request.version, PCP_VERSION);
	ASSERT_EQ(request.response, false);
	ASSERT_EQ(request.reserved_0, 0);
	ASSERT_EQ(request.opcode, pcp::Opcode::MAP);
	ASSERT_EQ(request.lifetime, 7200);
	ASSERT_EQ(request.client_ip, v6_bytes);

	const std::array<std::uint8_t, 12> nonce {
		0xf0, 0x6e, 0xee, 0xba, 0xd1, 0xf7,
		0x1c, 0x65, 0x4b, 0x26, 0x1f, 0x2f
	};

	const std::array<std::uint8_t, 16> ip {};

	const std::array<std::uint8_t, 3> reserved {
		0x00, 0x00, 0x00 
	};

	std::span<const std::uint8_t> body_buff = {
		pcp_mapping_request.begin() + pcp::HEADER_SIZE, pcp_mapping_request.end()
	};

	const auto body = deserialise<pcp::MapRequest>(body_buff);
	ASSERT_EQ(body.internal_port, 8085);
	ASSERT_EQ(body.suggested_external_port, 8085);
	ASSERT_EQ(body.suggested_external_ip, ip);
	ASSERT_EQ(body.nonce, nonce);
	ASSERT_EQ(body.reserved_0, reserved);
}

TEST_F(Ports, PCP_TestResponseVector) {
	const std::array<std::uint8_t, 12> res_1{};
	const auto header = deserialise<pcp::ResponseHeader>(pcp_mapping_response);
	ASSERT_EQ(header.version, PCP_VERSION);
	ASSERT_EQ(header.reserved_0, 0);
	ASSERT_EQ(header.reserved_1, res_1);
	ASSERT_EQ(header.epoch_time, 783343);
	ASSERT_EQ(header.opcode, pcp::Opcode::MAP);
	ASSERT_EQ(header.lifetime, 120);
	ASSERT_EQ(header.response, true);

	const std::array<std::uint8_t, 12> nonce {
		0xf0, 0x6e, 0xee, 0xba, 0xd1, 0xf7,
		0x1c, 0x65, 0x4b, 0x26, 0x1f, 0x2f
	};

	const auto address = boost::asio::ip::address::from_string("::ffff:209.244.0.3");
	const auto v6_bytes = address.to_v6().to_bytes();

	const std::array<std::uint8_t, 3> reserved {};

	std::span<const std::uint8_t> body_buff = {
		pcp_mapping_response.begin() + pcp::HEADER_SIZE, pcp_mapping_response.end()
	};

	const auto body = deserialise<pcp::MapResponse>(body_buff);
	ASSERT_EQ(body.assigned_external_port, 8085);
	ASSERT_EQ(body.assigned_external_ip, v6_bytes);
	ASSERT_EQ(body.internal_port, 8085);
	ASSERT_EQ(body.protocol, pcp::Protocol::TCP);
	ASSERT_EQ(body.reserved, reserved);
	ASSERT_EQ(body.nonce, nonce);
}