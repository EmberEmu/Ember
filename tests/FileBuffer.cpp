/*
 * Copyright (c) 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/buffers/FileBuffer.h>
#include <shared/utility/FileMD5.h>
#include <boost/endian/conversion.hpp>
#include <gsl/util>
#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <cstdio>

using namespace ember;

TEST(FileBuffer, Read) {
	std::filesystem::path path("test_data/filebuffer");
	ASSERT_TRUE(std::filesystem::exists(path));
	
	spark::io::FileBuffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	std::uint8_t w = 0;
	std::uint16_t x = 0;
	std::uint32_t y = 0;
	std::uint64_t z = 0;

	std::string_view strcmp { "The quick brown fox jumped over the lazy dog." };
	std::string str_out;
	str_out.resize(strcmp.size());

	buffer.read(&w);
	buffer.read(&x);
	buffer.read(&y);
	buffer.read(&z);
	buffer.read(str_out.data(), str_out.size());

	ASSERT_TRUE(buffer) << "File read error occurred";

	boost::endian::little_to_native_inplace(x);
	boost::endian::little_to_native_inplace(y);
	boost::endian::little_to_native_inplace(z);

	ASSERT_EQ(w, 47) << "Wrong uint8 value";
	ASSERT_EQ(x, 49197) << "Wrong uint16 value";
	ASSERT_EQ(y, 2173709693) << "Wrong uint32 value";
	ASSERT_EQ(z, 1438110846748337907) << "Wrong uint64 value";
	ASSERT_EQ(str_out, strcmp);
}

TEST(FileBuffer, Write) {
	std::filesystem::path path { "tmp_unittest_FileBuffer_Write" };

	gsl::final_action fa([path] {
		std::filesystem::remove(path);
	});

	// in case previous clean-up failed
	std::filesystem::remove(path);

	spark::io::FileBuffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	std::uint8_t w = 47;
	std::uint16_t x = 49197;
	std::uint32_t y = 2173709693;
	std::uint64_t z = 1438110846748337907;
	std::string str { "The quick brown fox jumped over the lazy dog."};

	boost::endian::native_to_little_inplace(x);
	boost::endian::native_to_little_inplace(y);
	boost::endian::native_to_little_inplace(z);

	buffer.write(w);
	buffer.write(x);
	buffer.write(y);
	buffer.write(z);
	buffer.write(str.data(), str.size() + 1); // write terminator
	buffer.flush(); // ensure data has been written before the read

	const auto md5_1 = util::generate_md5("test_data/filebuffer");
	const auto md5_2 = util::generate_md5(path);
	ASSERT_EQ(md5_1, md5_2);
}

TEST(FileBuffer, Copy) {
	std::filesystem::path path("test_data/filebuffer");
	ASSERT_TRUE(std::filesystem::exists(path));

	spark::io::FileBuffer buffer(path);

	std::uint8_t in = 0;
	buffer.copy(&in);
	ASSERT_EQ(in, 47);
	buffer.copy(&in);
	ASSERT_EQ(in, 47);
	buffer.read(&in); // advance to next byte without skip
	ASSERT_EQ(in, 47);
	buffer.copy(&in);
	ASSERT_EQ(in, 45);
}

TEST(FileBuffer, Skip) {
	std::filesystem::path path("test_data/filebuffer");
	ASSERT_TRUE(std::filesystem::exists(path));

	spark::io::FileBuffer buffer(path);

	{
		std::uint8_t in = 0;
		buffer.read(&in);
		ASSERT_EQ(in, 47);
		buffer.skip(1);
		buffer.read(&in);
		ASSERT_EQ(in, 192);
	}

	buffer.skip(4);

	{
		std::uint32_t in = 0;
		buffer.read(&in);
		ASSERT_EQ(in, 403842803);
	}

	ASSERT_TRUE(buffer) << "Read failure";
}

TEST(FileBuffer, InitialSize) {
	std::filesystem::path path("test_data/filebuffer");
	ASSERT_TRUE(std::filesystem::exists(path));

	spark::io::FileBuffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";
	ASSERT_EQ(buffer.size(), 61) << "Wrong size";
}

TEST(FileBuffer, ReadWriteMix) {
	std::filesystem::path path { "tmp_unittest_FileBuffer_ReadWriteMix" };

	gsl::final_action fa([path] {
		std::filesystem::remove(path);
	});

	// in case previous clean-up failed
	std::filesystem::remove(path);

	spark::io::FileBuffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	{
		std::uint8_t in = 42;
		std::uint8_t out = 0;
		buffer.write(in);
		buffer.read(&out);
		ASSERT_EQ(in, out);
	}

	{
		std::uint16_t in = 64245;
		std::uint16_t out = 0;
		buffer.write(in);
		buffer.read(&out);
		ASSERT_EQ(in, out);
	}

	{
		std::uint32_t in = 80144;
		std::uint32_t out = 0;
		buffer.write(in);
		buffer.read(&out);
		ASSERT_EQ(in, out);
	}

	{
		std::uint64_t in = 1438110846748337907;
		std::uint64_t out = 0;
		buffer.write(in);
		buffer.read(&out);
		ASSERT_EQ(in, out);
	}

	{
		std::uint16_t x_in = 60925;
		std::uint16_t y_in = 1352;
		std::uint16_t x_out = 0;
		std::uint16_t y_out = 0;
		buffer.write(x_in);
		buffer.write(y_in);
		buffer.read(&x_out);
		buffer.read(&y_out);
		ASSERT_EQ(x_in, x_out);
		ASSERT_EQ(y_in, y_out);
	}
}

TEST(FileBuffer, FindFirstOf) {
	std::filesystem::path path("test_data/filebuffer");

	ASSERT_TRUE(std::filesystem::exists(path));

	spark::io::FileBuffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	EXPECT_EQ(buffer.find_first_of(0x2f), 0);
	EXPECT_EQ(buffer.find_first_of(0x20), 18);
	EXPECT_EQ(buffer.find_first_of(0x6f), 27);
	EXPECT_EQ(buffer.find_first_of(0x6a), 35);
	EXPECT_EQ(buffer.find_first_of(0x00), 60);
	EXPECT_EQ(buffer.find_first_of(0xff), spark::io::FileBuffer::npos);
	ASSERT_TRUE(buffer) << "Error occurred during seeking";
}