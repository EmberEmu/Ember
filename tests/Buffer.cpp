/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/buffers/ChainedNode.h>
#include <gtest/gtest.h>
#include <memory>

namespace spark = ember::spark;

TEST(BufferTest, Size) {
	const int iterations = 5;
	spark::BufferBlock<sizeof(int) * iterations> buffer;
	int foo = 24221;
	std::size_t written = 0;

	for(int i = 0; i < 5; ++i) {
		written += buffer.write(reinterpret_cast<const char*>(&foo), sizeof(int));
	}

	ASSERT_EQ(sizeof(int) * iterations, written) << "Number of bytes written is incorrect";
	ASSERT_EQ(sizeof(int) * iterations, buffer.size()) << "Buffer size is incorrect";

	// attempt to exceed capacity - write should return 0
	written = buffer.write(reinterpret_cast<const char*>(&foo), sizeof(int));
	ASSERT_EQ(0, written) << "Number of bytes written is incorrect";
	ASSERT_EQ(sizeof(int) * iterations, buffer.size()) << "Buffer size is incorrect";
}

TEST(BufferTest, ReadWriteConsistency) {
	const char text[] = "The quick brown fox jumps over the lazy dog";
	spark::BufferBlock<sizeof(text)> buffer;

	std::size_t written = buffer.write(text, sizeof(text));
	ASSERT_EQ(sizeof(text), written) << "Incorrect write size";

	char text_out[sizeof(text)];

	std::size_t read = buffer.read(text_out, sizeof(text));
	ASSERT_EQ(sizeof(text), read) << "Incorrect read size";

	ASSERT_STREQ(text, text_out) << "Read produced incorrect result";
	ASSERT_EQ(0, buffer.size()) << "Buffer should be empty";
}


TEST(BufferTest, Skip) {
	const char text[] = "The quick brown fox jumps over the lazy dog";
	spark::BufferBlock<sizeof(text)> buffer;

	buffer.write(text, sizeof(text));
	auto text_out = std::make_unique<char[]>(sizeof(text));

	std::size_t skipped = buffer.skip(4);
	ASSERT_EQ(4, skipped) << "Incorrect skip length";
	
	buffer.read(text_out.get(), sizeof(text) - 4);
	ASSERT_STREQ("quick brown fox jumps over the lazy dog", text_out.get())
		<< "Skip/read produced incorrect result";
}