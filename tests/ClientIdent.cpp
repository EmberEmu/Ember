/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/ClientIdent.h>
#include <gtest/gtest.h>
#include <array>
#include <random>

using namespace ember;

struct ClientIdentTest : public testing::Test {
	static void SetUpTestSuite() {
		constexpr auto bits = std::numeric_limits<std::uint64_t>::digits;
		std::independent_bits_engine<std::default_random_engine, bits, std::uint64_t> engine;
		std::ranges::generate(rng::xorshift::seed, engine);
	}
};

TEST_F(ClientIdentTest, DefaultCtor) {
	ClientIdent ident;
	ASSERT_TRUE(ident.is_zero());
}

TEST_F(ClientIdentTest, IndexCtor) {
	ClientIdent ident(0);
	ASSERT_FALSE(ident.is_zero());
}

TEST_F(ClientIdentTest, SetZero) {
	ClientIdent ident(0);
	ASSERT_FALSE(ident.is_zero());
	ident.set_zero();
	ASSERT_TRUE(ident.is_zero());
}

TEST_F(ClientIdentTest, Index) {
	ClientIdent ident(5);
	ASSERT_EQ(ident.service(), 5);
}

TEST_F(ClientIdentTest, Hash) {
	const std::array<std::uint8_t, 16> data {
		0xf1, 0xb9, 0xa0, 0x02, 0x21, 0xe6, 0xb9, 0x7c,
		0x81, 0x5d, 0x1a, 0xd6, 0x98, 0xc1, 0xc3, 0x0a
	};

	ClientIdent ident;
	ASSERT_EQ(ident.hash(), 0x69691905);
	// hash is cached, make sure it's still correct the second time
	ASSERT_EQ(ident.hash(), 0x69691905);
}

TEST_F(ClientIdentTest, ExtractPtr) {
	ClientIdent ident(0);
	void* foo = (void*)0xbadf00d;
	ident.encode(foo, 0);
	const auto ptr = ident.extract_ptr<void*>();
	ASSERT_EQ(foo, ptr);
}

TEST_F(ClientIdentTest, ExtractSlot) {
	ClientIdent ident(0);
	const auto expected = 4091u;
	ident.encode(nullptr, expected);
	const auto val = ident.extract_slot();
	ASSERT_EQ(expected, val);
}

TEST_F(ClientIdentTest, ExtractPtrAndSlot) {
	ClientIdent ident(0);
	void* foo = (void*)0xbadf00d;
	const auto expected = 4091u;
	ident.encode(foo, expected);
	const auto ptr = ident.extract_ptr<void*>();
	const auto val = ident.extract_slot();
	ASSERT_EQ(foo, ptr);
	ASSERT_EQ(expected, val);
}

TEST_F(ClientIdentTest, Size) {
	ASSERT_EQ(ClientIdent::size(), 16);
}

TEST_F(ClientIdentTest, CopyCtorEquality) {
	const std::array<std::uint8_t, 16> data {
		0xf1, 0xb9, 0xa0, 0x02, 0x21, 0xe6, 0xb9, 0x7c,
		0x81, 0x5d, 0x1a, 0xd6, 0x98, 0xc1, 0xc3, 0x0a
	};

	ClientIdent ident;
	ClientIdent ident2(ident);
	ASSERT_EQ(ident, ident2);
}

TEST_F(ClientIdentTest, CopyAssignEquality) {
	const std::array<std::uint8_t, 16> data {
		0xf1, 0xb9, 0xa0, 0x02, 0x21, 0xe6, 0xb9, 0x7c,
		0x81, 0x5d, 0x1a, 0xd6, 0x98, 0xc1, 0xc3, 0x0a
	};

	ClientIdent ident;
	ClientIdent ident2 = ident;
	ASSERT_EQ(ident, ident2);
}

TEST_F(ClientIdentTest, CtorEquality) {
	const std::array<std::uint8_t, 16> data {
		0xf1, 0xb9, 0xa0, 0x02, 0x21, 0xe6, 0xb9, 0x7c,
		0x81, 0x5d, 0x1a, 0xd6, 0x98, 0xc1, 0xc3, 0x0a
	};

	ClientIdent ident(data);
	ClientIdent ident2(data);
	ASSERT_EQ(ident, ident2);
}
