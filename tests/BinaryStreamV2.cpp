/*
 * Copyright (c) 2018 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/buffers/DynamicBuffer.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/BufferAdaptor.h>
#include <gtest/gtest.h>
#include <gsl/gsl_util>
#include <algorithm>
#include <array>
#include <chrono>
#include <numeric>
#include <random>
#include <cstdint>
#include <cstdlib>

namespace spark = ember::spark;

TEST(BinaryStreamV2, MessageReadLimit) {
	std::array<std::uint8_t, 14> ping {
		0x00, 0x0C, 0xDC, 0x01, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00
	};

	// write the ping packet data twice to the buffer
	spark::io::DynamicBuffer<32> buffer;
	buffer.write(ping.data(), ping.size());
	buffer.write(ping.data(), ping.size());

	// read one packet back out (reuse the ping array)
	spark::io::BinaryStream stream(buffer, ping.size());
	ASSERT_EQ(stream.read_limit(), ping.size());
	ASSERT_NO_THROW(stream.get(ping.data(), ping.size()))
		<< "Failed to read packet back from stream";

	// attempt to read past the stream message bound
	ASSERT_THROW(stream.get(ping.data(), ping.size()), spark::io::stream_read_limit)
		<< "Message boundary was not respected";
	ASSERT_EQ(stream.state(), spark::io::StreamState::READ_LIMIT_ERR)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, BufferLimit) {
	std::array<std::uint8_t, 14> ping {
		0x00, 0x0C, 0xDC, 0x01, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00
	};

	// write the ping packet data to the buffer
	spark::io::DynamicBuffer<32> buffer;
	buffer.write(ping.data(), ping.size());

	// read all data back out
	spark::io::BinaryStream stream(buffer);
	ASSERT_NO_THROW(stream.get(ping.data(), ping.size()))
		<< "Failed to read packet back from stream";

	// attempt to read past the buffer bound
	ASSERT_THROW(stream.get(ping.data(), ping.size()), spark::io::buffer_underrun)
		<< "Message boundary was not respected";
	ASSERT_EQ(stream.state(), spark::io::StreamState::BUFF_LIMIT_ERR)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, ReadWriteInts) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);

	const std::uint16_t in { 100 };
	stream << in;

	ASSERT_EQ(stream.size(), sizeof(in));
	ASSERT_EQ(stream.size(), buffer.size());

	std::uint16_t out;
	stream >> out;

	ASSERT_EQ(in, out);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(buffer.empty());
	ASSERT_EQ(stream.state(), spark::io::StreamState::OK)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, ReadWriteStdString) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string in { "The quick brown fox jumped over the lazy dog" };
	stream << in;

	// +1 to account for the terminator that's written
	ASSERT_EQ(stream.size(), in.size() + 1);

	std::string out;
	stream >> out;

	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(in, out);
	ASSERT_EQ(stream.state(), spark::io::StreamState::OK)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, ReadWriteCString) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);
	const char* in { "The quick brown fox jumped over the lazy dog" };
	stream << in;

	ASSERT_EQ(stream.size(), strlen(in) + 1);

	std::string out;
	stream >> out;

	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(0, strcmp(in, out.c_str()));
	ASSERT_EQ(stream.state(), spark::io::StreamState::OK)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, ReadWriteVector) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);

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
	ASSERT_EQ(stream.state(), spark::io::StreamState::OK)
		<< "Unexpected stream state";
}

TEST(BinaryStreamV2, Clear) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);
	stream << 0xBADF00D;

	ASSERT_TRUE(!stream.empty());
	ASSERT_TRUE(!buffer.empty());

	stream.skip(stream.size());

	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(buffer.empty());
}

TEST(BinaryStreamV2, Skip) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);

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
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);
	ASSERT_EQ(buffer.can_write_seek(), stream.can_write_seek());
}

TEST(BinaryStreamV2, GetPut) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);
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
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	stream.fill<30>(128);
	ASSERT_EQ(buffer.size(), 30);
	ASSERT_EQ(stream.total_write(), 30);
	stream.fill<2>(128);
	ASSERT_EQ(buffer.size(), 32);
	ASSERT_EQ(stream.total_write(), 32);
	auto it = std::find_if(buffer.begin(), buffer.end(),  [](auto i) { return i != 128; });
	ASSERT_EQ(it, buffer.end());
}

TEST(BinaryStreamV2, NoCopyStringRead) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	const std::uint32_t trailing { 0x0DDBA11 };
	stream << input << trailing;

	// check this stream uses a contiguous buffer
	const auto contig = std::is_same<decltype(stream)::contiguous_type, spark::io::is_contiguous>::value;
	ASSERT_TRUE(contig);

	// find the end of the string within the buffer
	const auto stream_buf = stream.buffer();
	const auto pos = stream_buf->find_first_of('\0');
	ASSERT_NE(pos, adaptor.npos);

	// create a view into the buffer & skip ahead so the next read continues as normal
	std::string_view output(stream_buf->read_ptr(), pos);
	ASSERT_EQ(input, output);

	// ensure we can still read subsequent data as normal
	stream.skip(pos + 1); // +1 to skip terminator
	std::uint32_t trailing_output = 0;
	stream >> trailing_output ;
	ASSERT_EQ(trailing, trailing_output);
}

TEST(BinaryStreamV2, StringViewRead) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	const std::uint32_t trailing { 0x0DDBA11 };
	stream << input << trailing;

	auto view = stream.view();
	ASSERT_EQ(input, view);

	// ensure we can still read subsequent data as normal
	std::uint32_t trailing_output = 0;
	stream >> trailing_output;
	ASSERT_EQ(trailing, trailing_output);
}

TEST(BinaryStreamV2, PartialStringViewRead) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << input;

	auto span = stream.span(20);
	std::string_view view(span);
	ASSERT_EQ("The quick brown fox ", view);

	// read the rest of the string
	view = stream.view();
	ASSERT_EQ("jumped over the lazy dog", view);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStreamV2, StringViewStream) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	const std::uint32_t trailing { 0xDEFECA7E };
	stream << input << trailing;

	std::string_view output;
	stream >> output;
	ASSERT_EQ(input, output);

	// ensure we can still read subsequent data as normal
	std::uint32_t trailing_output = 0;
	stream >> trailing_output;
	ASSERT_EQ(trailing, trailing_output);
	
	// make a sly modification to the buffer and check the string_view matches
	buffer[0] = 'A';
	ASSERT_EQ(buffer[0], output[0]);
	ASSERT_NE(input, output);
}

TEST(BinaryStreamV2, Array) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	const int arr[] = { 1, 2, 3 }; 
	stream << arr;
	int val = 0;
	stream >> val;
	ASSERT_EQ(val, 1);
	stream >> val;
	ASSERT_EQ(val, 2);
	stream >> val;
	ASSERT_EQ(val, 3);
}

TEST(BinaryStreamV2, Span) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	const int arr[] = { 4, 9, 2, 1 }; // chosen by fair dice roll
	stream << arr;
	auto span = stream.span<int>(4);
	ASSERT_TRUE(stream.empty());
	// not checking directly against the array in case it somehow gets clobbered,
	// which would mean the span would get clobbered and succeed where it shouldn't
	ASSERT_EQ(span[0], 4);
	ASSERT_EQ(span[1], 9);
	ASSERT_EQ(span[2], 2);
	ASSERT_EQ(span[3], 1);
}
