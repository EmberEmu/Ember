/*
 * Copyright (c) 2018 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/buffers/DynamicBuffer.h>
#include <spark/buffers/FileBuffer.h>
#include <spark/buffers/StaticBuffer.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/BufferAdaptor.h>
#include <shared/utility/cstring_view.hpp>
#include <shared/utility/FileMD5.h>
#include <boost/endian/conversion.hpp>
#include <gtest/gtest.h>
#include <gsl/narrow>
#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <list>
#include <format>
#include <numeric>
#include <random>
#include <set>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <cstring>

using namespace ember;

TEST(BinaryStream, MessageReadLimit) {
	std::array<std::uint8_t, 14> ping {
		0x00, 0x0C, 0xDC, 0x01, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00
	};

	// write the ping packet data twice to the buffer
	spark::io::DynamicBuffer<32> buffer;
	buffer.write(ping);
	buffer.write(ping);

	// read one packet back out (reuse the ping array)
	spark::io::BinaryStream stream(buffer, ping.size());
	ASSERT_EQ(stream.read_limit(), ping.size());
	ASSERT_NO_THROW(stream.get(ping))
		<< "Failed to read packet back from stream";

	// attempt to read past the stream message bound
	ASSERT_THROW(stream.get(ping.data(), ping.size()), spark::io::stream_read_limit)
		<< "Message boundary was not respected";
	ASSERT_EQ(stream.state(), spark::io::StreamState::read_limit_error)
		<< "Unexpected stream state";
}

TEST(BinaryStream, BufferLimit) {
	std::array<std::uint8_t, 14> ping {
		0x00, 0x0C, 0xDC, 0x01, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00
	};

	// write the ping packet data to the buffer
	spark::io::DynamicBuffer<32> buffer;
	buffer.write(ping);

	// read all data back out
	spark::io::BinaryStream stream(buffer);
	ASSERT_NO_THROW(stream.get(ping))
		<< "Failed to read packet back from stream";

	// attempt to read past the buffer bound
	ASSERT_THROW(stream.get(ping), spark::io::buffer_underrun)
		<< "Message boundary was not respected";
	ASSERT_EQ(stream.state(), spark::io::StreamState::buffer_limit_error)
		<< "Unexpected stream state";
}

TEST(BinaryStream, ReadWriteInts) {
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
	ASSERT_EQ(stream.state(), spark::io::StreamState::ok)
		<< "Unexpected stream state";
}

TEST(BinaryStream, ReadWriteStdString) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string in { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::null_terminated(in);

	ASSERT_EQ(stream.size(), in.size() + 1); // +1, account for the terminator

	std::string out;
	stream >> spark::io::null_terminated(out);

	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(in, out);
	ASSERT_EQ(stream.state(), spark::io::StreamState::ok)
		<< "Unexpected stream state";
}

TEST(BinaryStream, ReadWriteCString) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);
	const char* in { "The quick brown fox jumped over the lazy dog" };
	stream << in;

	ASSERT_EQ(stream.size(), strlen(in) + 1);

	std::string out;
	stream >> spark::io::null_terminated(out);

	ASSERT_TRUE(stream.empty());
	ASSERT_EQ(0, strcmp(in, out.c_str()));
	ASSERT_EQ(stream.state(), spark::io::StreamState::ok)
		<< "Unexpected stream state";
}

TEST(BinaryStream, ReadWriteVector) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);

	const auto time = std::chrono::system_clock::now().time_since_epoch();
	const unsigned int seed = gsl::narrow_cast<unsigned int>(time.count());
	std::srand(seed);

	std::vector<int> in(std::rand() % 200);
	std::ranges::iota(in, std::rand() % 100);
	std::ranges::shuffle(in, std::default_random_engine(seed));

	stream.put(in);

	ASSERT_EQ(stream.size(), in.size() * sizeof(int));

	// read the integers back one by one, making sure they
	// match the expected value
	for(auto value : in) {
		int output;
		stream >> output;
		ASSERT_EQ(output, value);
	}

	stream.put(in);
	std::vector<int> out(in.size());

	// read the integers to an output buffer and compare both
	stream.get(out);
	ASSERT_EQ(in, out);
	ASSERT_EQ(stream.state(), spark::io::StreamState::ok)
		<< "Unexpected stream state";
}

TEST(BinaryStream, Clear) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);
	stream << 0xBADF00D;

	ASSERT_TRUE(!stream.empty());
	ASSERT_TRUE(!buffer.empty());

	stream.skip(stream.size());

	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(buffer.empty());
}

TEST(BinaryStream, Skip) {
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

TEST(BinaryStream, CanWriteSeek) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);
	ASSERT_EQ(buffer.can_write_seek(), stream.can_write_seek());
}

TEST(BinaryStream, GetPut) {
	spark::io::DynamicBuffer<32> buffer;
	spark::io::BinaryStream stream(buffer);
	std::vector<std::uint8_t> in { 1, 2, 3, 4, 5 };
	std::vector<std::uint8_t> out(in.size());

	stream.put(in);
	stream.get(out);

	ASSERT_EQ(stream.total_read(), out.size());
	ASSERT_EQ(stream.total_write(), in.size());
	ASSERT_EQ(in, out);
}

TEST(BinaryStream, Fill) {
	std::vector<std::uint8_t> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	stream.fill<30>(128);
	ASSERT_EQ(buffer.size(), 30);
	ASSERT_EQ(stream.total_write(), 30);
	stream.fill<2>(128);
	ASSERT_EQ(buffer.size(), 32);
	ASSERT_EQ(stream.total_write(), 32);
	auto it = std::ranges::find_if(buffer,  [](auto i) { return i != 128; });
	ASSERT_EQ(it, buffer.end());
}

TEST(BinaryStream, NoCopyStringRead) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	const std::uint32_t trailing { 0x0DDBA11 };
	stream << spark::io::null_terminated(input) << trailing;

	// check this stream uses a contiguous buffer
	const auto contig = std::is_same_v<decltype(stream)::contiguous_type, spark::io::is_contiguous>;
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
	stream >> trailing_output;
	ASSERT_EQ(trailing, trailing_output);
}

TEST(BinaryStream, StringviewRead) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	const std::uint32_t trailing { 0x0DDBA11 };
	stream << spark::io::null_terminated(input) << trailing;

	auto view = stream.view();
	ASSERT_EQ(input, view);

	// ensure we can still read subsequent data as normal
	std::uint32_t trailing_output = 0;
	stream >> trailing_output;
	ASSERT_EQ(trailing, trailing_output);
}

TEST(BinaryStream, PartialStringviewRead) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::null_terminated(input);

	auto span = stream.span(20);
	std::string_view view(span);
	ASSERT_EQ("The quick brown fox ", view);

	// read the rest of the string
	view = stream.view();
	ASSERT_EQ("jumped over the lazy dog", view);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, StringviewStream) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	const std::uint32_t trailing { 0xDEFECA7E };
	stream << spark::io::null_terminated(input) << trailing;

	std::string_view output;
	stream >> spark::io::null_terminated(output);
	ASSERT_EQ(input, output);

	// ensure we can still read subsequent data as normal
	std::uint32_t trailing_output = 0;
	stream >> trailing_output;
	ASSERT_EQ(trailing, trailing_output);
	
	// make a sly modification to the buffer and check the string_view matches
	ASSERT_FALSE(buffer.empty());
	buffer[0] = 'A';
	ASSERT_EQ(buffer[0], output[0]);
	ASSERT_NE(input, output);
}

TEST(BinaryStream, Array) {
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

TEST(BinaryStream, Span) {
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

TEST(BinaryStream, StaticBufferWrite) {
	spark::io::StaticBuffer<char, 4> buffer;
	spark::io::BinaryStream stream(buffer);
	stream << 'a' << 'b' << 'c' << 'd';
	ASSERT_EQ(buffer[0], 'a');
	ASSERT_EQ(buffer[1], 'b');
	ASSERT_EQ(buffer[2], 'c');
	ASSERT_EQ(buffer[3], 'd');
	ASSERT_TRUE(stream);
}

TEST(BinaryStream, StaticBufferDirectWrite) {
	spark::io::StaticBuffer<char, 4> buffer;
	spark::io::BinaryStream stream(buffer);
	std::uint32_t input = 0xBEE5BEE5;
	std::uint32_t output = 0;
	buffer.write(&input, sizeof(input));
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream);
}

TEST(BinaryStream, StaticBufferOverflow) {
	spark::io::StaticBuffer<char, 4> buffer;
	spark::io::BinaryStream stream(buffer);
	ASSERT_THROW(stream << std::uint64_t(1), spark::io::buffer_overflow);
	ASSERT_FALSE(stream);
}

TEST(BinaryStream, StaticBufferRead) {
	spark::io::StaticBuffer<char, 4> buffer;
	const std::uint32_t input = 0x11223344;
	buffer.write(&input, sizeof(input));
	spark::io::BinaryStream stream(buffer);
	std::uint32_t output = 0;
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream);
}

TEST(BinaryStream, StaticBufferUnderrun) {
	spark::io::StaticBuffer<char, 4> buffer;
	spark::io::BinaryStream stream(buffer);
	std::uint32_t input = 0xBEEFBEEF;
	std::uint32_t output = 0;
	stream << input;
	stream >> output;
	ASSERT_THROW(stream >> output, spark::io::buffer_underrun);
	ASSERT_FALSE(stream);
	ASSERT_EQ(input, output);
}

TEST(BinaryStream, StaticBufferUnderrunNoExcept) {
	spark::io::StaticBuffer<char, 4> buffer;
	spark::io::BinaryStream stream(buffer, spark::io::no_throw);
	std::uint32_t output = 0;
	stream << output;
	stream >> output;
	ASSERT_NO_THROW(stream >> output);
	ASSERT_FALSE(stream);
	ASSERT_EQ(output, 0);
}

TEST(BinaryStream, static_buffer_adaptor_regression) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(buffer);
	stream << 1 << 2 << 3;

	std::string foo { "foo " };
	stream << foo;

	std::string_view sv;
	stream.skip(sizeof(int) * 3);
	stream >> sv;
	ASSERT_TRUE(stream);
}

TEST(BinaryStream, static_buffer_adaptor_exception_regression) {
	spark::io::StaticBuffer<char, 16> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(buffer);
	std::string_view str { "This is a string that is longer than the size of the buffer..." };
	ASSERT_THROW(stream << str, spark::exception);
	ASSERT_FALSE(stream);
}

TEST(BinaryStream, StaticBufferAdaptor_NoExceptionRegression) {
	spark::io::StaticBuffer<char, 16> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(buffer, spark::io::no_throw);
	std::string_view str { "This is a string that is longer than the size of the buffer..." };
	ASSERT_NO_THROW(stream << str);
	ASSERT_FALSE(stream);

}

TEST(BinaryStream, PutIntegralLiterals) {
	spark::io::StaticBuffer<char, 64> buffer;
	spark::io::BinaryStream stream(buffer);
	stream.put<std::uint64_t>(std::numeric_limits<std::uint64_t>::max());
	stream.put<std::uint32_t>(std::numeric_limits<std::uint32_t>::max());
	stream.put<std::uint16_t>(std::numeric_limits<std::uint16_t>::max());
	stream.put<std::uint8_t>(std::numeric_limits<std::uint8_t>::max());
	stream.put<std::int64_t>(std::numeric_limits<std::int64_t>::max());
	stream.put<std::int32_t>(std::numeric_limits<std::int32_t>::max());
	stream.put<std::int16_t>(std::numeric_limits<std::int16_t>::max());
	stream.put<std::int8_t>(std::numeric_limits<std::int8_t>::max());
	stream.put(1.5f);
	stream.put(3.0);
	std::uint64_t resultu64 = 0;
	std::uint32_t resultu32 = 0;
	std::uint16_t resultu16 = 0;
	std::uint8_t resultu8 = 0;
	std::int64_t result64 = 0;
	std::int32_t result32 = 0;
	std::int16_t result16 = 0;
	std::int8_t result8 = 0;
	float resultf = 0.0f;
	double resultd = 0.0;
	stream >> resultu64 >> resultu32 >> resultu16 >> resultu8;
	stream >> result64 >> result32 >> result16 >> result8;
	stream >> resultf >> resultd;
	ASSERT_FLOAT_EQ(1.5f, resultf);
	ASSERT_DOUBLE_EQ(3.0, resultd);
	ASSERT_EQ(resultu8, std::numeric_limits<std::uint8_t>::max());
	ASSERT_EQ(resultu16, std::numeric_limits<std::uint16_t>::max());
	ASSERT_EQ(resultu32, std::numeric_limits<std::uint32_t>::max());
	ASSERT_EQ(resultu64, std::numeric_limits<std::uint64_t>::max());
	ASSERT_EQ(result8, std::numeric_limits<std::int8_t>::max());
	ASSERT_EQ(result16, std::numeric_limits<std::int16_t>::max());
	ASSERT_EQ(result32, std::numeric_limits<std::int32_t>::max());
	ASSERT_EQ(result64, std::numeric_limits<std::int64_t>::max());
	ASSERT_TRUE(stream);
}

TEST(BinaryStream, FileBufferRead) {
	std::filesystem::path path("test_data/filebuffer");
	ASSERT_TRUE(std::filesystem::exists(path));

	spark::io::FileBuffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	spark::io::BinaryStream stream(buffer);
	std::uint8_t w = 0;
	std::uint16_t x = 0;
	std::uint32_t y = 0;
	std::uint64_t z = 0;

	std::string_view strcmp { "The quick brown fox jumped over the lazy dog." };
	std::string str_out;

	stream >> w >> x >> y >> z >> spark::io::null_terminated(str_out);
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

TEST(BinaryStream, FileBufferWrite) {
	std::filesystem::path path { "tmp_unittest_BinaryStream_FileBufferWrite" };

	gsl::final_action fa([path] {
		std::filesystem::remove(path);
	});

	// in case previous clean-up failed
	std::filesystem::remove(path);

	spark::io::FileBuffer buffer(path);
	ASSERT_TRUE(buffer) << "File open failed";

	spark::io::BinaryStream stream(buffer);

	std::uint8_t w = 47;
	std::uint16_t x = 49197;
	std::uint32_t y = 2173709693;
	std::uint64_t z = 1438110846748337907;
	std::string str { "The quick brown fox jumped over the lazy dog."};

	boost::endian::native_to_little_inplace(x);
	boost::endian::native_to_little_inplace(y);
	boost::endian::native_to_little_inplace(z);

	stream << w << x << y << z << spark::io::null_terminated(str);
	buffer.flush(); // ensure data is written before following read

	const auto md5_1 = utility::generate_md5("test_data/filebuffer");
	const auto md5_2 = utility::generate_md5(path);
	ASSERT_EQ(md5_1, md5_2);
}

TEST(BinaryStream, SetErrorState) {
	spark::io::StaticBuffer<char, 64> buffer;
	spark::io::BinaryStream stream(buffer);
	ASSERT_TRUE(stream);
	ASSERT_TRUE(stream.good());
	ASSERT_TRUE(stream.state() == spark::io::StreamState::ok);
	stream.set_error_state();
	ASSERT_FALSE(stream);
	ASSERT_FALSE(stream.good());
	ASSERT_TRUE(stream.state() == spark::io::StreamState::user_defined_error);
}

TEST(BinaryStream, StringAdaptor_PrefixedVarint_Long) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	std::string input;

	// encode varint requiring three bytes
	input.resize_and_overwrite(80'000, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});
	
	stream << spark::io::prefixed_varint(input);
	std::string output;
	stream >> spark::io::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(BinaryStream, StringAdaptor_PrefixedVarint_Medium) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	std::string input;

	// encode varint requiring two bytes
	input.resize_and_overwrite(5'000, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});
	
	stream << spark::io::prefixed_varint(input);
	std::string output;
	stream >> spark::io::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(BinaryStream, StringAdaptor_PrefixedVarint_Short) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	std::string input;

	// encode varint requiring one byte
	input.resize_and_overwrite(127, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});
	
	stream << spark::io::prefixed_varint(input);
	std::string output;
	stream >> spark::io::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(BinaryStream, StringAdaptor_Prefixed) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::prefixed(input);
	std::string output;
	stream >> spark::io::prefixed(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, StringAdaptor_Default) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << input;
	std::string output;
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, StringAdaptor_Raw) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const auto input = std::format("String with {} embedded null", '\0');
	stream << spark::io::raw(input);
	ASSERT_EQ(input.size(), buffer.size());
	std::string output;
	stream >> spark::io::null_terminated(output);
	ASSERT_EQ(output, "String with ");
	ASSERT_FALSE(stream.empty());
}

TEST(BinaryStream, StringAdaptor_NullTerminated) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string input { "We're just normal strings. Innocent strings."};
	stream << spark::io::null_terminated(input);
	std::string output;
	stream >> spark::io::null_terminated(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, StringviewAdaptor_PrefixedVarint_Long) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	std::string input;

	// encode varint requiring three bytes
	input.resize_and_overwrite(80'000, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});

	stream << spark::io::prefixed_varint(input);
	std::string_view output;
	stream >> spark::io::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(BinaryStream, StringviewAdaptor_PrefixedVarint_Medium) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	std::string input;

	// encode varint requiring two bytes
	input.resize_and_overwrite(5'000, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});

	stream << spark::io::prefixed_varint(input);
	std::string_view output;
	stream >> spark::io::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(BinaryStream, StringviewAdaptor_PrefixedVarint_Short) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	std::string input;

	// encode varint requiring one byte
	input.resize_and_overwrite(127, [&](char* buffer, const std::size_t size) {
		for(std::size_t i = 0; i < size; ++i) {
			buffer[i] = (rand() % 127) + 32; // ASCII a-z
		}

		return size;
	});

	stream << spark::io::prefixed_varint(input);
	std::string_view output;
	stream >> spark::io::prefixed_varint(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
}

TEST(BinaryStream, StringviewAdaptor_Prefixed) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	std::string_view input { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::prefixed(input);
	std::string_view output;
	stream >> spark::io::prefixed(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, StringviewAdaptor_Default) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	std::string_view input { "The quick brown fox jumped over the lazy dog" };
	stream << input;
	std::string_view output;
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, StringviewAdaptor_Raw) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const auto input = std::format("String with {} embedded null", '\0');
	stream << spark::io::raw(input);
	ASSERT_EQ(input.size(), buffer.size());
	std::string_view output;
	stream >> spark::io::null_terminated(output);
	ASSERT_EQ(output, "String with ");
	ASSERT_FALSE(stream.empty());
}

TEST(BinaryStream, StringviewAdaptor_NullTerminated) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	std::string_view input { "We're just normal strings. Innocent strings."};
	stream << spark::io::null_terminated(input);
	ASSERT_EQ(stream.size(), input.size() + 1); // +1, account for the terminator
	std::string_view output;
	stream >> spark::io::null_terminated(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, StdArray) {
	std::array<char, 128> buffer{};
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);
	std::string_view input { "We're just normal strings. Innocent strings."};

	// array is considered full by default as size == capacity
	ASSERT_THROW(stream << input, spark::io::buffer_overflow);
	adaptor.clear();
	stream.clear_error_state();

	// try again now we've reset the state
	std::string_view output;
	stream << input;
	stream >> output;
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, TotalWriteConsistency) {
	std::array<char, 256> buffer;
	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor);

	ASSERT_EQ(stream.total_write(), 0);
	stream << std::uint8_t(0);
	ASSERT_EQ(stream.total_write(), 1);
	stream << std::uint16_t(0);
	ASSERT_EQ(stream.total_write(), 3);
	stream << std::uint32_t(0);
	ASSERT_EQ(stream.total_write(), 7);
	stream << std::uint64_t(0);
	ASSERT_EQ(stream.total_write(), 15);

	std::string_view str { "hello, world!" };
	stream << spark::io::raw(str);
	ASSERT_EQ(stream.total_write(), 28);
	stream << spark::io::prefixed(str);
	ASSERT_EQ(stream.total_write(), 45);
	stream << spark::io::prefixed_varint(str);
	ASSERT_EQ(stream.total_write(), 59);
	stream << spark::io::null_terminated(str);
	ASSERT_EQ(stream.total_write(), 73);

	stream.put<std::uint8_t>(0);
	ASSERT_EQ(stream.total_write(), 74);
	stream.put<std::uint16_t>(0);
	ASSERT_EQ(stream.total_write(), 76);
	stream.put<std::uint32_t>(0);
	ASSERT_EQ(stream.total_write(), 80);
	stream.put<std::uint64_t>(0);
	ASSERT_EQ(stream.total_write(), 88);

	std::array<std::uint32_t, 4> data {};
	stream.put(data);
	ASSERT_EQ(stream.total_write(), 104);
	stream.put(data);
	ASSERT_EQ(stream.total_write(), 120);
	stream.put(data);
	ASSERT_EQ(stream.total_write(), 136);
}

TEST(BinaryStream, EndianessBigMatch) {
	std::array<char, 16> buffer{};
	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);
	std::uint64_t input = 100, output = 0;
	stream << std::uint64_t(100);
	stream >> output;
	ASSERT_EQ(input, output);
}

TEST(BinaryStream, EndiannessLittleMatch) {
	std::array<char, 16> buffer{};
	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::little);
	std::uint64_t input = 100, output = 0;
	stream << std::uint64_t(100);
	stream >> output;
	ASSERT_EQ(input, output);
}

TEST(BinaryStream, EndiannessNativeMatch) {
	std::array<char, 16> buffer{};
	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::native);
	std::uint64_t input = 100, output = 0;
	stream << std::uint64_t(100);
	stream >> output;
	ASSERT_EQ(input, output);
}

TEST(BinaryStream, EndiannessBigExplicitMatch) {
	std::array<char, 16> buffer{};
	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);
	std::uint64_t input = 100, output = 0;
	stream << spark::io::endian::le(input);
	stream >> spark::io::endian::le(output);
	ASSERT_EQ(input, output);
	stream << spark::io::endian::be(input);
	stream >> spark::io::endian::be(output);
	ASSERT_EQ(input, output);
}

TEST(BinaryStream, EndiannessLittleExplicitMatch) {
	std::array<char, 16> buffer{};
	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::little);
	std::uint64_t input = 100, output = 0;
	stream << spark::io::endian::be(input);
	stream >> spark::io::endian::be(output);
	ASSERT_EQ(input, output);
	stream << spark::io::endian::le(input);
	stream >> spark::io::endian::le(output);
	ASSERT_EQ(input, output);
}

TEST(BinaryStream, EndiannessNativeExplicitMatch) {
	std::array<char, 16> buffer{};
	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::native);
	std::uint64_t input = 100, output = 0;
	stream << spark::io::endian::be(input);
	stream >> spark::io::endian::be(output);
	ASSERT_EQ(input, output);
	stream << spark::io::endian::be(input);
	stream >> spark::io::endian::be(output);
	ASSERT_EQ(input, output);
}

TEST(BinaryStream, EndiannessBigExplicitMismatch) {
	std::array<char, 16> buffer{};
	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::big);
	std::uint64_t input = 100, output = 0;
	stream << spark::io::endian::le(input);
	stream >> spark::io::endian::be(output);
	ASSERT_NE(input, output);
	stream << spark::io::endian::be(input);
	stream >> spark::io::endian::le(output);
	ASSERT_NE(input, output);
}

TEST(BinaryStream, EndiannessLittleExplicitMismatch) {
	std::array<char, 16> buffer{};
	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::little);
	std::uint64_t input = 100, output = 0;
	stream << spark::io::endian::le(input);
	stream >> spark::io::endian::be(output);
	ASSERT_NE(input, output);
	stream << spark::io::endian::be(input);
	stream >> spark::io::endian::le(output);
	ASSERT_NE(input, output);
}

TEST(BinaryStream, EndiannessNativeExplicitMismatch) {
	std::array<char, 16> buffer{};
	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor, spark::io::endian::native);
	std::uint64_t input = 100, output = 0;
	stream << spark::io::endian::le(input);
	stream >> spark::io::endian::be(output);
	ASSERT_NE(input, output);
	stream << spark::io::endian::be(input);
	stream >> spark::io::endian::le(output);
	ASSERT_NE(input, output);
}

namespace {

struct Foo {
	std::uint16_t x;
	std::uint32_t y;
	std::uint64_t z;
	std::string_view str;

	void serialise(auto& stream) {
		stream(x, y, z, spark::io::null_terminated(str));
		stream & spark::io::endian::be(x);
	}

	bool operator==(const Foo& rhs) const {
		return x == rhs.x && y == rhs.y && z == rhs.z && str == rhs.str;
	}
};

}

TEST(BinaryStream, ExperimentalSerialise) {
	std::array<char, 32> buffer{};

	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor);

	Foo input {
		.x = 100,
		.y = 200,
		.z = 300,
		.str = { "It's a fake!"  }
	};

	stream.serialise(input);
	ASSERT_EQ(stream.total_write(), 29);

	Foo output{};
	ASSERT_NE(input, output);
	stream.deserialise(output);
	ASSERT_EQ(stream.total_read(), 29);
	ASSERT_TRUE(stream.empty());
	ASSERT_TRUE(stream);
	ASSERT_EQ(input, output);
}

namespace {

struct Complex {
	std::string str = "Hello, world!";
	std::vector<int> vec { 1, 2, 3, 4, 5 };
	std::list<int> list { 6, 7, 8, 9, 10 };
	std::set<int> set { 11, 12, 13, 14, 15 };

	void serialise(auto& stream) {
		stream(str, vec, list, set);
	}
};

}

TEST(BinaryStream, IterableContainers) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	std::vector primitives { 1, 2, 3, 4, 5 };
	stream << primitives;
	ASSERT_EQ(primitives.size() * sizeof(int), stream.total_write());
	ASSERT_EQ(primitives.size() * sizeof(int), adaptor.size());
	ASSERT_EQ(primitives.size() * sizeof(int), buffer.size());

	std::vector<std::string> strings { "hello, ", "world!" };
	stream << strings;

	ASSERT_EQ(stream.get<int>(), 1);
	ASSERT_EQ(stream.get<int>(), 2);
	ASSERT_EQ(stream.get<int>(), 3);
	ASSERT_EQ(stream.get<int>(), 4);
	ASSERT_EQ(stream.get<int>(), 5);

	std::string hello, world;
	stream >> hello >> world;
	ASSERT_EQ(hello, "hello, ");
	ASSERT_EQ(world, "world!");

	Complex obj_in;
	stream << obj_in;

	std::string out;
	stream >> out;
	ASSERT_EQ(obj_in.str, out);
	ASSERT_EQ(stream.get<int>(), 1);
	ASSERT_EQ(stream.get<int>(), 2);
	ASSERT_EQ(stream.get<int>(), 3);
	ASSERT_EQ(stream.get<int>(), 4);
	ASSERT_EQ(stream.get<int>(), 5);
	ASSERT_EQ(stream.get<int>(), 6);
	ASSERT_EQ(stream.get<int>(), 7);
	ASSERT_EQ(stream.get<int>(), 8);
	ASSERT_EQ(stream.get<int>(), 9);
	ASSERT_EQ(stream.get<int>(), 10);
	ASSERT_EQ(stream.get<int>(), 11);
	ASSERT_EQ(stream.get<int>(), 12);
	ASSERT_EQ(stream.get<int>(), 13);
	ASSERT_EQ(stream.get<int>(), 14);
	ASSERT_EQ(stream.get<int>(), 15);
}

namespace {

struct PrefixedContainers {
	std::vector<int> vec;
	std::list<int> list;

	void serialise(auto& stream) {
		stream & spark::io::prefixed(vec);
		stream & spark::io::prefixed(list);
	}

	bool operator==(const PrefixedContainers& rhs) const {
		return vec == rhs.vec && list == rhs.list;
	}
};

}

TEST(BinaryStream, PrefixedContainers) {
	std::vector<char> buffer;
	spark::io::BufferAdaptor adaptor(buffer);
	spark::io::BinaryStream stream(adaptor);

	std::vector primitives { 1, 2, 3, 4, 5 };
	stream << spark::io::prefixed(primitives);

	std::vector<int> output;
	stream >> spark::io::prefixed(output);
	ASSERT_TRUE(std::ranges::equal(primitives, output));

	std::vector<PrefixedContainers> objects;

	for(int i = 0u; i < 5; ++i) {
		objects.emplace_back(PrefixedContainers{ { 1 + i, 2 + i }, { 3 + i, 4 + i} });
	}

	stream << spark::io::prefixed(objects);

	std::vector<PrefixedContainers> output_objs;
	stream >> spark::io::prefixed(output_objs);
	EXPECT_EQ(objects.size(), output_objs.size());
	ASSERT_EQ(objects, output_objs);
}

TEST(BinaryStream, StdArrayTest) {
	std::array<char, 16> buffer;
	spark::io::BufferAdaptor adaptor(buffer, spark::io::init_empty);
	spark::io::BinaryStream stream(adaptor);
	ASSERT_TRUE(adaptor.empty());
	ASSERT_EQ(adaptor.size(), 0);
	ASSERT_EQ(stream.size(), 0);
}

TEST(BinaryStream, StringAdaptor_Prefixed1b) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::prefixed<const std::string, std::uint8_t>(input);
	std::string output;
	stream >> spark::io::prefixed<std::string, std::uint8_t>(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, StringAdaptor_Prefixed2b) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::prefixed<const std::string, std::uint16_t>(input);
	std::string output;
	stream >> spark::io::prefixed<std::string, std::uint16_t>(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, StringAdaptor_Prefixed4b) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::prefixed<const std::string, std::uint32_t>(input);
	std::string output;
	stream >> spark::io::prefixed<std::string, std::uint32_t>(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, StringAdaptor_Prefixed8b) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::prefixed<const std::string, std::uint64_t>(input);
	std::string output;
	stream >> spark::io::prefixed<std::string, std::uint64_t>(output);
	ASSERT_EQ(input, output);
	ASSERT_TRUE(stream.empty());
}

TEST(BinaryStream, StringAdaptor_PrefixedBigEndian) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::prefixed<const std::string, std::uint32_t, spark::io::endian::as_big_t>(input);
	std::uint32_t read{};
	stream >> spark::io::endian::be(read);
	ASSERT_EQ(input.size(), read);
}

TEST(BinaryStream, StringAdaptor_PrefixedLittleEndian) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::prefixed<const std::string, std::uint32_t, spark::io::endian::as_little_t>(input);
	std::uint32_t read{};
	stream >> spark::io::endian::le(read);
	ASSERT_EQ(input.size(), read);
}

TEST(BinaryStream, StringAdaptor_PrefixedNativeEndian) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::prefixed<const std::string, std::uint32_t, spark::io::endian::as_native_t>(input);
	std::uint32_t read{};
	stream >> read;
	ASSERT_EQ(input.size(), read);
}

TEST(BinaryStream, StringAdaptor_PrefixedEndianMismatch) {
	spark::io::StaticBuffer<char, 128> buffer;
	spark::io::BinaryStream stream(buffer);
	const std::string input { "The quick brown fox jumped over the lazy dog" };
	stream << spark::io::prefixed<const std::string, std::uint32_t, spark::io::endian::as_big_t>(input);
	std::uint32_t read{};
	stream >> spark::io::endian::le(read);
	ASSERT_NE(input.size(), read);
}