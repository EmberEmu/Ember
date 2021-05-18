/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mdns/Parser.h>
#include <gtest/gtest.h>
#include <cstdint>

namespace dns = ember::dns;

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
TEST(DNSParsing, FlagsEncode) {
	const std::uint16_t expected = 0b0101'1'0'1'0'1'0'1'0001'1;

	dns::Flags flags {
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
TEST(DNSParsing, FlagsDecode) {
	const std::uint16_t flags = 0b0101'1'0'1'0'1'0'1'0001'1;;
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
TEST(DNSParsing, FlagsRoundtrip) {
	const std::uint16_t flags = rand() % std::numeric_limits<std::uint16_t>::max();
	const auto decoded = dns::Parser::decode_flags(flags);
	const auto output = dns::Parser::encode_flags(decoded);
	EXPECT_EQ(flags, output) << "Header flags mismatch after decode -> encode round-trip";
}