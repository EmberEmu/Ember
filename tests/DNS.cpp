/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mdns/Parser.h>
#include <gtest/gtest.h>
#include <array>
#include <cstdint>

namespace dns = ember::dns;

constexpr auto DNS_HEADER_SIZE = 12u;
constexpr auto DNS_MAX_PAYLOAD_SIZE = 9000u; // should include UDP & IPvx headers

// random real-world multicast query
constexpr std::array<std::uint8_t, 40> valid_query
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x5F, 0x67, 0x6F,
	0x6F, 0x67, 0x6C, 0x65, 0x63, 0x61, 0x73, 0x74, 0x04, 0x5F, 0x74, 0x63, 0x70, 0x05, 0x6C, 0x6F,
	0x63, 0x61, 0x6C, 0x00, 0x00, 0x0C, 0x00, 0x01
};

// flags  layout
// qr - 0
// op - 1 (4 bits)
// aa - 5
// tc - 6
// rd - 7
// ra - 8
//  z - 9
// ad - 10
// cd - 11
// rc - 12 (4 bits)

// Use a pre-populated Flags structure and ensure the encoded
// flag matches the predetermined value
TEST(DNSParser, FlagsEncode) {
	const std::uint16_t expected = 0b0101'1'0'1'0'1'0'1'0001'1;

	const dns::Flags flags {
		.qr = 1,
		.opcode = dns::Opcode::IQUERY,
		.aa = 1,
		.tc = 0,
		.rd = 1,
		.ra = 0,
		.z = 1,
		.ad = 0,
		.cd = 1,
		.rcode = dns::ReplyCode::REFUSED
	};
	
	const auto encoded = dns::Parser::encode_flags(flags);
	EXPECT_EQ(encoded, expected);
}

// Use a predetermined set of flags and ensure the Flags
// structure output matches the input value
TEST(DNSParser, FlagsDecode) {
	const std::uint16_t flags = 0b0101'1'0'1'0'1'0'1'0001'1;
	const auto decoded = dns::Parser::decode_flags(flags);
	EXPECT_EQ(decoded.qr, 1);
	EXPECT_EQ(decoded.opcode, dns::Opcode::IQUERY);
	EXPECT_EQ(decoded.aa, 1);
	EXPECT_EQ(decoded.tc, 0);
	EXPECT_EQ(decoded.rd, 1);
	EXPECT_EQ(decoded.ra, 0);
	EXPECT_EQ(decoded.z, 1);
	EXPECT_EQ(decoded.ad, 0);
	EXPECT_EQ(decoded.cd, 1);
	EXPECT_EQ(decoded.rcode, dns::ReplyCode::REFUSED);
}

// Generate a random set of flags and ensure the value can be decoded
// and encoded, producing the same set of flags at the end
TEST(DNSParser, FlagsRoundtrip) {
	const std::uint16_t flags = rand() % std::numeric_limits<std::uint16_t>::max();
	const auto decoded = dns::Parser::decode_flags(flags);
	const auto output = dns::Parser::encode_flags(decoded);
	EXPECT_EQ(flags, output) << "Header flags mismatch after decode -> encode round-trip";
}

// validate a random real-world mdns query header
TEST(DNSParser, HeaderOverlay) {
	/*const auto header = dns::Parser::header_overlay(valid_query);
	EXPECT_EQ(header->id, 0) << "Incorrect query ID";
	EXPECT_EQ(header->questions, 1) << "Incorrect number of questions";
	EXPECT_EQ(header->authority_rrs, 0) << "Incorrect authority RRs";
	EXPECT_EQ(header->answers, 0) << "Incorrect number of answers";
	EXPECT_EQ(header->additional_rrs, 0) << "Incorrect number of additional RRs";
	*/// todo, readd flags test
}

//// validate a random real-world mdns query
//TEST(DNSParser, ValidateQuery) {
//	const auto res = dns::Parser::validate(valid_query);
//	EXPECT_EQ(res, dns::Result::OK);
//}

// intentionally don't pass enough data for a valid header
TEST(DNSParser, Parser_HeaderBounds) {
	constexpr std::array<std::uint8_t, DNS_HEADER_SIZE> header { 0 };
	const auto res = dns::Parser::validate({ header.data(), header.size() - 1 });
	EXPECT_EQ(res, dns::Result::HEADER_TOO_SMALL);
}

// intentionally pass too much data for a valid payload
//TEST(DNSParser, Parser_PayloadBounds) {
//	constexpr std::array<std::uint8_t, DNS_MAX_PAYLOAD_SIZE + 1> payload { 0 };
//	const auto res = dns::Parser::validate(payload);
//	EXPECT_EQ(res, dns::Result::PAYLOAD_TOO_LARGE);
//}