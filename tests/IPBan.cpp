/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/IPBanCache.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

class IPBanTest : public ::testing::Test {
public:
	virtual void SetUp() {
		std::vector<ember::IPEntry> entries {
			{ "198.51.106.51",   8 },
			{ "172.16.125.134", 16 },
			{ "169.254.26.21",  24 },
			{ "203.62.113.82",  31 },
			{ "192.88.99.62",   32 }
		};

		bans = std::make_unique<ember::IPBanCache>(entries);
	}

	virtual void TearDown() {}

	std::unique_ptr<ember::IPBanCache> bans;
};


TEST_F(IPBanTest, Mask32) {
	EXPECT_FALSE(bans->is_banned("192.88.99.61"))
		<< "Banned below range";;
	EXPECT_TRUE(bans->is_banned("192.88.99.62"));
	EXPECT_FALSE(bans->is_banned("192.88.99.63"))
		<< "Banned above range";;
}

TEST_F(IPBanTest, Mask31) {
	EXPECT_FALSE(bans->is_banned("203.62.113.81"))
		<< "Banned below range";;
	EXPECT_TRUE(bans->is_banned("203.62.113.82"));
	EXPECT_TRUE(bans->is_banned("203.62.113.83"));
	EXPECT_FALSE(bans->is_banned("203.62.113.84"))
		<< "Banned above range";
}

TEST_F(IPBanTest, Mask24) {
	EXPECT_FALSE(bans->is_banned("169.254.25.255"))
		<< "Banned below range";;
	EXPECT_TRUE(bans->is_banned("169.254.26.0"));
	EXPECT_TRUE(bans->is_banned("169.254.26.21"));
	EXPECT_TRUE(bans->is_banned("169.254.26.3"));
	EXPECT_TRUE(bans->is_banned("169.254.26.128"));
	EXPECT_TRUE(bans->is_banned("169.254.26.255"));
	EXPECT_FALSE(bans->is_banned("169.254.27.0"))
		<< "Banned above range";
}

TEST_F(IPBanTest, Mask16) {
	EXPECT_FALSE(bans->is_banned("172.15.255.255"))
		<< "Banned below range";;
	EXPECT_TRUE(bans->is_banned("172.16.0.0"));
	EXPECT_TRUE(bans->is_banned("172.16.125.134"));
	EXPECT_TRUE(bans->is_banned("172.16.117.92"));
	EXPECT_TRUE(bans->is_banned("172.16.4.92"));
	EXPECT_TRUE(bans->is_banned("172.16.255.255"));
	EXPECT_FALSE(bans->is_banned("172.17.0.0"))
		<< "Banned above range";
}

TEST_F(IPBanTest, Mask8) {
	EXPECT_FALSE(bans->is_banned("197.255.255.255"))
		<< "Banned below range";
	EXPECT_TRUE(bans->is_banned("198.0.0.0"));
	EXPECT_TRUE(bans->is_banned("198.51.106.51"));
	EXPECT_TRUE(bans->is_banned("198.51.106.162"));
	EXPECT_TRUE(bans->is_banned("198.51.42.162"));
	EXPECT_TRUE(bans->is_banned("198.43.42.162"));
	EXPECT_TRUE(bans->is_banned("198.255.255.255"));
	EXPECT_FALSE(bans->is_banned("199.0.0.0"))
		<< "Banned above range";
}