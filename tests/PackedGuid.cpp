/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <protocol/types/PackedGuid.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/BufferAdaptor.h>
#include <gtest/gtest.h>
#include <cstdlib>
#include <ctime>

using namespace ember;

// ensure only the mask byte is stored
TEST(PackedGuid, RoundTripEmpty) {
	protocol::PackedGuid input(0);
	std::vector<std::uint8_t> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	
	// test input
	stream << input;
	ASSERT_EQ(stream.size(), 1); // mask byte only

	// test output - set to a random value to avoid incidental equality
	protocol::PackedGuid output(0xcafebeefcafebeef);
	stream >> output;
	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(input, output);
}

// set only the LSB
TEST(PackedGuid, RoundTripOneByte) {
	protocol::PackedGuid input(1);
	std::vector<std::uint8_t> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	// test input
	stream << input;
	ASSERT_EQ(stream.size(), 2); // mask byte & one byte

	// test output - set to a random value to avoid incidental equality
	protocol::PackedGuid output;
	stream >> output;
	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(input, output);
}

// set only the MSB
TEST(PackedGuid, RoundTripHiByte) {
	protocol::PackedGuid input(0x8000000000000000);
	std::vector<std::uint8_t> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	// test input
	stream << input;
	ASSERT_EQ(stream.size(), 2); // mask byte & one byte

	// test output
	protocol::PackedGuid output;
	stream >> output;
	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(input, output);
}

// set only the MSB & LSB
TEST(PackedGuid, RoundTripHiLoByte) {
	protocol::PackedGuid input(0x8000000000000001);
	std::vector<std::uint8_t> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	// test input
	stream << input;
	ASSERT_EQ(stream.size(), 3); // mask byte & two bytes

	// test output
	protocol::PackedGuid output;
	stream >> output;
	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(input, output);
}

// set the MSB in each byte to ensure they're all streamed
TEST(PackedGuid, RoundTripHiBitPerByte) {
	protocol::PackedGuid input(0x8080808080808080);
	std::vector<std::uint8_t> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	// test input
	stream << input;
	ASSERT_EQ(stream.size(), 9); // mask byte & all bytes

	// test output
	protocol::PackedGuid output;
	stream >> output;
	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(input, output);
}

// set the LSB in each byte to ensure they're all streamed
TEST(PackedGuid, RoundTripLoBitPerByte) {
	protocol::PackedGuid input(0x0101010101010101);
	std::vector<std::uint8_t> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	// test input
	stream << input;
	ASSERT_EQ(stream.size(), 9); // mask byte & all bytes

	// test output
	protocol::PackedGuid output;
	stream >> output;
	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(input, output);
}

// set one bit in every byte to ensure they're all streamed
TEST(PackedGuid, RoundTripAllBits) {
	protocol::PackedGuid input(0xffffffffffffffff);
	std::vector<std::uint8_t> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	// test input
	stream << input;
	ASSERT_EQ(stream.size(), 9); // mask byte & all bytes

	// test output
	protocol::PackedGuid output;
	stream >> output;
	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(input, output);
}

TEST(PackedGuid, RoundTripRandomGuid) {
	srand(time(nullptr));
	protocol::PackedGuid input(rand());
	std::vector<std::uint8_t> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	stream << input;

	// test output
	protocol::PackedGuid output;
	stream >> output;
	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(input, output);
}

// ensure detection of potentially malicious PackedGuids
// where the mask indicates more bytes are set than are
// actually sent in the message
TEST(PackedGuid, TruncatedPackedGuidNoThrow) {
	std::uint8_t mask = 0x7f;
	std::vector<std::uint8_t> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor, spark::io::no_throw);
	stream << mask;
	stream << std::uint8_t(1);

	protocol::PackedGuid output;
	stream >> output;

	// ensure stream has detected a bad state
	ASSERT_TRUE(stream.empty());
	ASSERT_FALSE(stream);
}

// same as above but with exceptions enabled
TEST(PackedGuid, TruncatedPackedGuidThrow) {
	std::uint8_t mask = 0x7f;
	std::vector<std::uint8_t> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	stream << mask;
	stream << std::uint8_t(1);

	protocol::PackedGuid output;
	ASSERT_ANY_THROW(stream >> output);
}