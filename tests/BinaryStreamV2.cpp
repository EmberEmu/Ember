/*
 * Copyright (c) 2018 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/buffers/BinaryStream.h>
#include <spark/v2/buffers/BufferAdaptor.h>
#include <spark/buffers/DynamicBuffer.h>
#include <gtest/gtest.h>
#include <gsl/gsl_util>
#include <algorithm>
#include <array>
#include <chrono>
#include <numeric>
#include <random>
#include <cstdint>
#include <ctime>
#include <cstdlib>

namespace spark = ember::spark;

TEST(BinaryStreamV2, MessageReadLimit) {
	std::array<std::uint8_t, 14> ping {
		0x00, 0x0C, 0xDC, 0x01, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00
	};

	// write the ping packet data twice to the buffer
	spark::DynamicBuffer<32> buffer;
	buffer.write(ping.data(), ping.size());
	buffer.write(ping.data(), ping.size());

	// read one packet back out (reuse the ping array)
	spark::v2::BinaryStream stream(buffer, ping.size());
	ASSERT_EQ(stream.read_limit(), ping.size());
	ASSERT_NO_THROW(stream.get(ping.data(), ping.size()))
		<< "Failed to read packet back from stream";

	// attempt to read past the stream message bound
	ASSERT_THROW(stream.get(ping.data(), ping.size()), spark::stream_read_limit)
		<< "Message boundary was not respected";
	ASSERT_EQ(stream.state(), spark::StreamState::READ_LIMIT_ERR)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, BufferLimit) {
	std::array<std::uint8_t, 14> ping {
		0x00, 0x0C, 0xDC, 0x01, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00
	};

	// write the ping packet data to the buffer
	spark::DynamicBuffer<32> buffer;
	buffer.write(ping.data(), ping.size());

	// read all data back out
	spark::v2::BinaryStream stream(buffer);
	ASSERT_NO_THROW(stream.get(ping.data(), ping.size()))
		<< "Failed to read packet back from stream";

	// attempt to read past the buffer bound
	ASSERT_THROW(stream.get(ping.data(), ping.size()), spark::buffer_underrun)
		<< "Message boundary was not respected";
	ASSERT_EQ(stream.state(), spark::StreamState::BUFF_LIMIT_ERR)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, ReadWriteInts) {
	spark::DynamicBuffer<32> buffer;
	spark::v2::BinaryStream stream(buffer);

	const std::uint16_t in { 100 };
	stream << in;

	ASSERT_EQ(stream.size(), sizeof(in));
	ASSERT_EQ(stream.size(), buffer.size());

	std::uint16_t out;
	stream >> out;

	ASSERT_EQ(in, out);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(buffer.empty());
	ASSERT_EQ(stream.state(), spark::StreamState::OK)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, ReadWriteStdString) {
	spark::DynamicBuffer<32> buffer;
	spark::v2::BinaryStream stream(buffer);
	const std::string in { "The quick brown fox jumped over the lazy dog" };
	stream << in;

	// +1 to account for the terminator that's written
	ASSERT_EQ(stream.size(), in.size() + 1);

	std::string out;
	stream >> out;

	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(in, out);
	ASSERT_EQ(stream.state(), spark::StreamState::OK)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, ReadWriteCString) {
	spark::DynamicBuffer<32> buffer;
	spark::v2::BinaryStream stream(buffer);
	const char* in { "The quick brown fox jumped over the lazy dog" };
	stream << in;

	// no terminator is written for a C string
	// bit inconsistent with std::string handling?
	ASSERT_EQ(stream.size(), strlen(in));

	std::string out;
	stream >> out;

	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(0, strcmp(in, out.c_str()));
	ASSERT_EQ(stream.state(), spark::StreamState::OK)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, ReadWriteVector) {
	spark::DynamicBuffer<32> buffer;
	spark::v2::BinaryStream stream(buffer);

	const auto time = std::chrono::system_clock::now().time_since_epoch();
	const unsigned int seed = gsl::narrow_cast<unsigned int>(time.count());
	std::srand(seed);

	std::vector<int> in(std::rand() % 200);
	std::iota(in.begin(), in.end(), std::rand() % 100);
	std::shuffle(in.begin(), in.end(), std::default_random_engine(seed));

	stream.put(in.begin(), in.end());

	ASSERT_EQ(stream.size(), in.size() * sizeof(int));

	// read the integers back one by one, making sure they
	// match the expected value
	for(auto it = in.begin(); it != in.end(); ++it) {
		int output;
		stream >> output;
		ASSERT_EQ(output, *it);
	}

	stream.put(in.begin(), in.end());
	std::vector<int> out(in.size());

	// read the integers to an output buffer and compare both
	stream.get(out.begin(), out.end());
	ASSERT_EQ(in, out);
	ASSERT_EQ(stream.state(), spark::StreamState::OK)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, Clear) {
	spark::DynamicBuffer<32> buffer;
	spark::v2::BinaryStream stream(buffer);
	stream << 0xBADF00D;

	ASSERT_TRUE(!stream.empty());
	ASSERT_TRUE(!buffer.empty());

	stream.skip(stream.size());

	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(buffer.empty());
}

TEST(BinaryStreamV2, Skip) {
	spark::DynamicBuffer<32> buffer;
	spark::v2::BinaryStream stream(buffer);

	const std::uint64_t in {0xBADF00D};
	stream << in << in;
	stream.skip(sizeof(in));

	ASSERT_EQ(stream.size(), sizeof(in));
	ASSERT_EQ(stream.size(), buffer.size());

	std::uint64_t out;
	stream >> out;

	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(in, out);
}

TEST(BinaryStreamV2, CanWriteSeek) {
	spark::DynamicBuffer<32> buffer;
	spark::v2::BinaryStream stream(buffer);
	ASSERT_EQ(buffer.can_write_seek(), stream.can_write_seek());
}

TEST(BinaryStreamV2, GetPut) {
	spark::DynamicBuffer<32> buffer;
	spark::v2::BinaryStream stream(buffer);
	std::vector<std::uint8_t> in { 1, 2, 3, 4, 5 };
	std::vector<std::uint8_t> out(in.size());

	stream.put(in.data(), in.size());
	stream.get(out.data(), out.size());

	ASSERT_EQ(stream.total_read(), out.size());
	ASSERT_EQ(stream.total_write(), in.size());
	ASSERT_EQ(in, out);
}

TEST(BinaryStreamV2, Fill) {
	std::vector<std::uint8_t> buffer;
	spark::v2::BufferAdaptor adaptor(buffer);
	spark::v2::BinaryStream stream(adaptor);
	stream.fill<30>(128);
	ASSERT_EQ(buffer.size(), 30);
	ASSERT_EQ(stream.total_write(), 30);
	stream.fill<2>(128);
	ASSERT_EQ(buffer.size(), 32);
	ASSERT_EQ(stream.total_write(), 32);
	auto it = std::find_if(buffer.begin(), buffer.end(),  [](auto i) { return i != 128; });
	ASSERT_EQ(it, buffer.end());
}