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
#include <ports/upnp/HTTPHeaderParser.h>
#include <boost/endian.hpp>
#include <gtest/gtest.h>
#include <boost/asio/ip/address.hpp>
#include <array>
#include <limits>
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

TEST(PortsUPnP, HTTPHeader_Parse) {
	std::string_view header {
		R"(HTTP/1.1 200 OK)" "\r\n"
		R"(Cache-Control: max-age=120)" "\r\n"
		R"(Connection: Keep-Alive)" "\r\n"
		R"(Content-Length: 14813)" "\r\n"
		R"(Content-Type: text/xml)" "\r\n"
		R"(Date: Tue, 12 Mar 2024 21:42:45 GMT)" "\r\n"
		R"(ETag: "5B88239F8043B07A")" "\r\n"
		R"(Expires: Tue, 12 Mar 2024 21:44:45 GMT)" "\r\n"
		R"(Last-Modified: Fri, 02 Jun 2023 14:34:02 GMT)" "\r\n"
		R"(Mime-Version: 1.0)" "\r\n"
		R"(Keep-Alive: timeout=60, max=300)" "\r\n"
	};
	
	upnp::HTTPHeader parsed{};
	ASSERT_TRUE(upnp::parse_http_header(header, parsed));
	ASSERT_EQ(parsed.code, upnp::HTTPStatus::OK);
	ASSERT_EQ(parsed.fields["Cache-Control"], "max-age=120");
	ASSERT_EQ(parsed.fields["Connection"], "Keep-Alive");
	ASSERT_EQ(parsed.fields["Content-Length"], "14813");
	ASSERT_EQ(parsed.fields["Content-Type"], "text/xml");
	ASSERT_EQ(parsed.fields["Date"], "Tue, 12 Mar 2024 21:42:45 GMT");
	ASSERT_EQ(parsed.fields["ETag"], R"("5B88239F8043B07A")");
	ASSERT_EQ(parsed.fields["Expires"], "Tue, 12 Mar 2024 21:44:45 GMT");
	ASSERT_EQ(parsed.fields["Last-Modified"], "Fri, 02 Jun 2023 14:34:02 GMT");
	ASSERT_EQ(parsed.fields["Mime-Version"], "1.0");
	ASSERT_EQ(parsed.fields["Keep-Alive"], "timeout=60, max=300");


	header = std::string_view{
		R"(HTTP/1.1 404 Not Found)" "\r\n"
		R"(Access-Control-Allow-Origin: *)" "\r\n"
		R"(Connection: Keep-Alive)" "\r\n"
		R"(Content-Encoding: gzip)" "\r\n"
		R"(Content-Type: text/html; charset=utf-8)" "\r\n"
		R"(Date: Mon, 18 Jul 2016 16:06:00 GMT)" "\r\n"
		R"(Etag: "c561c68d0ba92bbeb8b0f612a9199f722e3a621a")" "\r\n"
		R"(Keep-Alive: timeout=5, max=997)" "\r\n"
		R"(Last-Modified: Mon, 18 Jul 2016 02:36:04 GMT)" "\r\n"
		R"(Server: Apache)" "\r\n"
		R"(Set-Cookie: mykey=myvalue; expires=Mon, 17-Jul-2017 16:06:00 GMT; Max-Age=31449600; Path=/; secure)" "\r\n"
		R"(Transfer-Encoding: chunked)" "\r\n"
		R"(Vary: Cookie, Accept-Encoding)" "\r\n"
		R"(X-Backend-Server: developer2.webapp.scl3.mozilla.com)" "\r\n"
		R"(X-Cache-Info: not cacheable; meta data too large)" "\r\n"
		R"(X-kuma-revision: 1085259)" "\r\n"
		R"(x-frame-options: DENY)" "\r\n"
	};
	
	parsed = {};
	ASSERT_TRUE(upnp::parse_http_header(header, parsed));
	ASSERT_EQ(parsed.code, upnp::HTTPStatus::NOT_FOUND);

	// make sure the lookup is case-insensitive
	ASSERT_EQ(parsed.fields["Access-Control-Allow-Origin"], "*");
	ASSERT_EQ(parsed.fields["aCcesS-COntroL-allow-origiN"], "*");
	ASSERT_EQ(parsed.fields["ACCESS-CONTROL-ALLOW-ORIGIN"], "*");
	ASSERT_EQ(parsed.fields["access-control-allow-origin"], "*");

	ASSERT_EQ(parsed.fields["Connection"], "Keep-Alive");
	ASSERT_EQ(parsed.fields["Content-Encoding"], "gzip");
	ASSERT_EQ(parsed.fields["Content-Type"], "text/html; charset=utf-8");
	ASSERT_EQ(parsed.fields["Date"], "Mon, 18 Jul 2016 16:06:00 GMT");
	ASSERT_EQ(parsed.fields["Etag"], R"("c561c68d0ba92bbeb8b0f612a9199f722e3a621a")");
	ASSERT_EQ(parsed.fields["Keep-Alive"], "timeout=5, max=997");
	ASSERT_EQ(parsed.fields["Last-Modified"], "Mon, 18 Jul 2016 02:36:04 GMT");
	ASSERT_EQ(parsed.fields["Server"], "Apache");
	ASSERT_EQ(parsed.fields["Set-Cookie"], "mykey=myvalue; expires=Mon, 17-Jul-2017 16:06:00 GMT; Max-Age=31449600; Path=/; secure");
	ASSERT_EQ(parsed.fields["Transfer-Encoding"], "chunked");
	ASSERT_EQ(parsed.fields["Vary"], "Cookie, Accept-Encoding");
	ASSERT_EQ(parsed.fields["X-Backend-Server"], "developer2.webapp.scl3.mozilla.com");
	ASSERT_EQ(parsed.fields["X-Cache-Info"], "not cacheable; meta data too large");
	ASSERT_EQ(parsed.fields["X-kuma-revision"], "1085259");
	ASSERT_EQ(parsed.fields["x-frame-options"], "DENY");
}

TEST(PortsUPnP, HTTPHeader_Invalid) {
	std::string_view header {
		R"(Cache-Control: max-age=120))" "\r\n"
		R"(Connection: Keep-Alive)" "\r\n"
		R"(Content-Length: 14813)" "\r\n"
		R"(Content-Type: text/xml)" "\r\n"
		R"(Date: Tue, 12 Mar 2024 21:42:45 GMT)" "\r\n"
		R"(ETag: "5B88239F8043B07A")" "\r\n"
		R"(Expires: Tue, 12 Mar 2024 21:44:45 GMT)" "\r\n"
		R"(Last-Modified: Fri, 02 Jun 2023 14:34:02 GMT)" "\r\n"
		R"(Mime-Version: 1.0)" "\r\n"
		R"(Keep-Alive: timeout=60, max=300)" "\r\n"
	};

	upnp::HTTPHeader parsed{};
	ASSERT_FALSE(upnp::parse_http_header(header, parsed));
}

TEST(PortsUPnP, SVToInt) {
	EXPECT_ANY_THROW(upnp::sv_to_int("test"));
	EXPECT_ANY_THROW(upnp::sv_to_int(" 1234567"));
	EXPECT_ANY_THROW(upnp::sv_to_int("1234567 "));
	EXPECT_ANY_THROW(upnp::sv_to_int("a1234567"));
	EXPECT_ANY_THROW(upnp::sv_to_int("1234567a"));
	EXPECT_ANY_THROW(upnp::sv_to_int("0xFFFF"));
	EXPECT_ANY_THROW(upnp::sv_to_int("0b00000001"));
	EXPECT_EQ(123456789, upnp::sv_to_int("0123456789"));
	EXPECT_EQ(0, upnp::sv_to_int("0"));
	constexpr auto int_max = std::numeric_limits<int>::max();
	const auto int_max_str = std::to_string(int_max);
	EXPECT_EQ(int_max, upnp::sv_to_int(int_max_str));
	constexpr auto int_min = std::numeric_limits<int>::min();
	const auto int_min_str = std::to_string(int_min);
	EXPECT_EQ(int_min, upnp::sv_to_int(int_min_str));
}

TEST(PortsUPnP, SVToLong) {
	constexpr auto max = std::numeric_limits<long>::max();
	const auto max_str = std::to_string(max);
	EXPECT_EQ(max, upnp::sv_to_long(max_str));
	constexpr auto min = std::numeric_limits<long>::min();
	const auto min_str = std::to_string(min);
	EXPECT_EQ(min, upnp::sv_to_long(min_str));
}

TEST(PortsUPnP, SVToLongLong) {
	constexpr auto max = std::numeric_limits<long long>::max();
	const auto max_str = std::to_string(max);
	EXPECT_EQ(max, upnp::sv_to_ll(max_str));
	constexpr auto min = std::numeric_limits<long long>::min();
	const auto min_str = std::to_string(min);
	EXPECT_EQ(min, upnp::sv_to_ll(min_str));
}