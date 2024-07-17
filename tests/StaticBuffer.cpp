/*
* Copyright (c) 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <spark/buffers/StaticBuffer.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <span>
#include <string_view>
#include <vector>
#include <cstdint>

using namespace std::literals;
using namespace ember;

TEST(StaticBuffer, InitialEmpty) {
	spark::io::StaticBuffer<char, 1> buffer;
	ASSERT_TRUE(buffer.empty());
	ASSERT_EQ(buffer.size(), 0);
}

TEST(StaticBuffer, InitialNotEmpty) {
	spark::io::StaticBuffer<char, 2> buffer { '1', '2' };
	ASSERT_FALSE(buffer.empty());
	ASSERT_EQ(buffer.size(), 2);
	ASSERT_EQ(buffer[0], '1');
	ASSERT_EQ(buffer[1], '2');
}

TEST(StaticBuffer, Empty) {
	spark::io::StaticBuffer<char, 1> buffer;
	ASSERT_TRUE(buffer.empty());
	char val = '\0';
	buffer.write(&val, 1);
	ASSERT_FALSE(buffer.empty());
}

TEST(StaticBuffer, ReadOne) {
	spark::io::StaticBuffer<char, 3> buffer { '1', '2', '3' };
	char value = 0;
	buffer.read(&value, 1);
	ASSERT_EQ(buffer.size(), 2);
	ASSERT_EQ(value, '1');
}

TEST(StaticBuffer, ReadAll) {
	spark::io::StaticBuffer<char, 3> buffer { '1', '2', '3' };
	std::array<char, 3> expected{ '1', '2', '3' };
	std::array<char, 3> values{};
	buffer.read(values.data(), values.size());
	ASSERT_TRUE(std::equal(values.begin(), values.end(), expected.begin()));
}

TEST(StaticBuffer, SingleSkipRead) {
	spark::io::StaticBuffer<char, 3> buffer { '1', '2', '3' };
	std::uint8_t value = 0;
	buffer.skip(1);
	buffer.read(&value, 1);
	ASSERT_EQ(buffer.size(), 1);
	ASSERT_EQ(buffer[0], '3');
}

TEST(StaticBuffer, MultiskipRead) {
	spark::io::StaticBuffer<char, 6> buffer { '1', '2', '3', '4', '5', '6' };
	std::uint8_t value = 0;
	buffer.skip(5);
	buffer.read(&value, 1);
	ASSERT_TRUE(buffer.empty());
	ASSERT_EQ(value, '6');
}

TEST(StaticBuffer, Write) {
	spark::io::StaticBuffer<std::uint8_t, 6> buffer;
	const std::array<std::uint8_t, 6> values { 1, 2, 3, 4, 5, 6 };
	buffer.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), values.size());
	ASSERT_EQ(buffer.size(), values.size());
	ASSERT_TRUE(std::equal(values.begin(), values.end(), buffer.begin()));
}

TEST(StaticBuffer, CanWriteSeek) {
	spark::io::StaticBuffer<char, 1> buffer;
	ASSERT_TRUE(buffer.can_write_seek());
}

TEST(StaticBuffer, WriteSeekBack) {
	spark::io::StaticBuffer<char, 3> buffer { '1', '2', '3' };
	std::array<char, 2> values { '5', '6' };
	buffer.write_seek(spark::io::BufferSeek::SK_BACKWARD, 2);
	buffer.write(values.data(), values.size());
	std::array<char, 3> expected { '1', '5', '6' };
	ASSERT_TRUE(std::equal(expected.begin(), expected.end(), buffer.begin()));
}

TEST(StaticBuffer, WriteSeekStart) {
	spark::io::StaticBuffer<char, 3> buffer { '1', '2', '3' };
	std::array<char, 3> values { '4', '5', '6' };
	buffer.write_seek(spark::io::BufferSeek::SK_ABSOLUTE, 0);
	buffer.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), values.size());
	ASSERT_TRUE(std::equal(buffer.begin(), buffer.end(), values.begin()));
}

TEST(StaticBuffer, ReadPtr) {
	spark::io::StaticBuffer<char, 3> buffer { '1', '2', '3' };
	auto ptr = buffer.read_ptr();
	ASSERT_EQ(*ptr, buffer[0]);
	buffer.skip(1);
	ptr = buffer.read_ptr();
	ASSERT_EQ(*ptr, buffer[0]);
	buffer.skip(1);
	ptr = buffer.read_ptr();
	ASSERT_EQ(*ptr, buffer[0]);
}

TEST(StaticBuffer, Subscript) {
	spark::io::StaticBuffer<char, 3> buffer { '1', '2', '3' };
	ASSERT_EQ(buffer[0], '1');
	ASSERT_EQ(buffer[1], '2');
	ASSERT_EQ(buffer[2], '3');
	buffer[0] = '4';
	ASSERT_EQ(buffer[0], '4');
	buffer[0] = '5';
	ASSERT_EQ(buffer[0], '5');
	ASSERT_EQ(buffer[0], buffer[0]);
}

TEST(StaticBuffer, FindFirstOf) {
	spark::io::StaticBuffer<char, 64> buffer;
	const auto str = "The quick brown fox jumped over the lazy dog"sv;
	buffer.write(str.data(), str.size());
	auto pos = buffer.find_first_of('\0');
	ASSERT_EQ(pos, buffer.npos); // direct string write is not terminated
	pos = buffer.find_first_of('g');
	ASSERT_EQ(pos, 43);
	pos = buffer.find_first_of('T');
	ASSERT_EQ(pos, 0);
	pos = buffer.find_first_of('t');
	ASSERT_EQ(pos, 32);
}

TEST(StaticBuffer, AdvanceWrite) {
	spark::io::StaticBuffer<char, 3> buffer { 'a', 'b', 'c' };
	buffer.write_seek(spark::io::BufferSeek::SK_ABSOLUTE, 0);
	const char val = 'd';
	buffer.advance_write(1);
	buffer.write(&val, 1);
	ASSERT_EQ(buffer[1], val);
}