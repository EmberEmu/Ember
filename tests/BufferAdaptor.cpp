/*
* Copyright (c) 2024 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <spark/buffers/pmr/BufferAdaptor.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <span>
#include <vector>
#include <cstdint>

using namespace ember;

TEST(BufferAdaptor, SizeEmptyInitial) {
	std::vector<std::uint8_t> buffer;
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	ASSERT_EQ(adaptor.size(), 0);
}

TEST(BufferAdaptor, Empty) {
	std::vector<std::uint8_t> buffer;
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	ASSERT_TRUE(adaptor.empty());
	buffer.emplace_back(1);
	ASSERT_FALSE(adaptor.empty());
}

TEST(BufferAdaptor, SizePopulatedInitial) {
	std::vector<std::uint8_t> buffer { 1 };
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	ASSERT_EQ(adaptor.size(), buffer.size());
}

TEST(BufferAdaptor, ResizeMatch) {
	std::vector<std::uint8_t> buffer { 1, 2, 3, 4, 5 };
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	ASSERT_EQ(adaptor.size(), buffer.size());
	buffer.emplace_back(6);
	ASSERT_EQ(adaptor.size(), buffer.size());
}

TEST(BufferAdaptor, ReadOne) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	std::uint8_t value = 0;
	adaptor.read(&value, 1);
	ASSERT_EQ(adaptor.size(), buffer.size() - 1);
	ASSERT_EQ(value, 1);
}

TEST(BufferAdaptor, ReadAll) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values{};
	adaptor.read(&values, values.size());
	ASSERT_TRUE(std::equal(buffer.begin(), buffer.begin(), values.begin()));
}

TEST(BufferAdaptor, SingleSkipRead) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	std::uint8_t value = 0;
	adaptor.skip(1);
	adaptor.read(&value, 1);
	ASSERT_EQ(adaptor.size(), 1);
	ASSERT_EQ(value, buffer[1]);
}

TEST(BufferAdaptor, MultiskipRead) {
	std::vector<std::uint8_t> buffer { 1, 2, 3, 4, 5, 6 };
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	std::uint8_t value = 0;
	adaptor.skip(5);
	adaptor.read(&value, 1);
	ASSERT_TRUE(adaptor.empty());
	ASSERT_EQ(value, buffer[5]);
}

TEST(BufferAdaptor, Write) {
	std::vector<std::uint8_t> buffer;
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	std::array<std::uint8_t, 6> values { 1, 2, 3, 4, 5, 6 };
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(adaptor.size(), values.size());
	ASSERT_EQ(buffer.size(), values.size());
	ASSERT_TRUE(std::equal(values.begin(), values.end(), buffer.begin()));
}

TEST(BufferAdaptor, WriteAppend) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values { 4, 5, 6 };
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), 6);
	ASSERT_EQ(adaptor.size(), buffer.size());
	std::array<std::uint8_t, 6> expected { 1, 2, 3, 4, 5, 6 };
	ASSERT_TRUE(std::equal(expected.begin(), expected.end(), buffer.begin()));
}

TEST(BufferAdaptor, CanWriteSeek) {
	std::vector<std::uint8_t> buffer;
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	ASSERT_TRUE(adaptor.can_write_seek());
}

TEST(BufferAdaptor, WriteSeekBack) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values { 4, 5, 6 };
	adaptor.write_seek(spark::io::BufferSeek::SK_BACKWARD, 2);
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), 4);
	ASSERT_EQ(adaptor.size(), buffer.size());
	std::array<std::uint8_t, 4> expected { 1, 4, 5, 6 };
	ASSERT_TRUE(std::equal(expected.begin(), expected.end(), buffer.begin()));
}

TEST(BufferAdaptor, WriteSeekStart) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	std::array<std::uint8_t, 3> values { 4, 5, 6 };
	adaptor.write_seek(spark::io::BufferSeek::SK_ABSOLUTE, 0);
	adaptor.write(values.data(), values.size());
	ASSERT_EQ(buffer.size(), values.size());
	ASSERT_EQ(adaptor.size(), buffer.size());
	ASSERT_TRUE(std::equal(buffer.begin(), buffer.end(), values.begin()));
}

TEST(BufferAdaptor, ReadPtr) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	auto ptr = adaptor.read_ptr();
	ASSERT_EQ(*ptr, buffer[0]);
	adaptor.skip(1);
	ptr = adaptor.read_ptr();
	ASSERT_EQ(*ptr, buffer[1]);
	adaptor.skip(1);
	ptr = adaptor.read_ptr();
	ASSERT_EQ(*ptr, buffer[2]);
}

TEST(BufferAdaptor, Subscript) {
	std::vector<std::uint8_t> buffer { 1, 2, 3 };
	spark::io::pmr::BufferAdaptor adaptor(buffer);
	ASSERT_EQ(adaptor[0], std::byte(1));
	ASSERT_EQ(adaptor[1], std::byte(2));
	ASSERT_EQ(adaptor[2], std::byte(3));
	buffer[0] = 4;
	ASSERT_EQ(adaptor[0], std::byte(4));
	adaptor[0] = std::byte(5);
	ASSERT_EQ(adaptor[0], std::byte(5));
	ASSERT_EQ(adaptor[0], std::byte(buffer[0]));
}