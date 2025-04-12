/*
 * Copyright (c) 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/NullBuffer.h>
#include <gtest/gtest.h>
#include <string_view>
#include <cstdint>

using namespace ember;

TEST(NullBuffer, WriteSize) {
	spark::io::NullBuffer buffer;
	spark::io::BinaryStream stream(buffer);

	stream << std::uint8_t(0);
	ASSERT_EQ(stream.total_write(), 1);
	stream << std::uint16_t(0);
	ASSERT_EQ(stream.total_write(), 3);
	stream << std::uint32_t(0);
	ASSERT_EQ(stream.total_write(), 7);
	stream << std::uint64_t(0);
	ASSERT_EQ(stream.total_write(), 15);

	std::string_view str { "A library serves no purpose unless someone is using it." };
	stream << spark::io::null_terminated(str);
	ASSERT_EQ(stream.total_write(), 71);
	ASSERT_TRUE(buffer.empty());
}