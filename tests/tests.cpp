/*
* Copyright (c) 2014 Ember
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <gtest/gtest.h>
#include <login/srp6/SRP6.h>
#include <botan/bigint.h>
#include <memory>

class SRP6ServerSessionTest : public ::testing::Test {
public:
	virtual void SetUp() {
		//Generate a username/password combo that we know the expected values for
		Botan::BigInt N{"0x894B645E89E1535BBDAD5B8B290650530801B18EBFBF5E8FAB3C82872A3E9BB7" };
		Botan::BigInt g{7};
		SRP6::Generator gen(g, N);
		SRP6::SVPair sv_pair(SRP6::generate_verifier("CHAOSVEX", "ABC", gen, 32));
		session = std::make_unique<SRP6::ServerSession>(gen, sv_pair.verifier, N);
	}

	virtual void TearDown() {

	}

	std::unique_ptr<SRP6::ServerSession> session;
};

TEST_F(SRP6ServerSessionTest, zero_ephemeral) {
	EXPECT_THROW(session->session_key(0), SRP6::exception) << "Public ephemeral key should never be zero!";
}

TEST_F(SRP6ServerSessionTest, negative_ephemeral) {
	EXPECT_THROW(session->session_key(Botan::BigInt("-10")), SRP6::exception)
		<< "Public ephemeral key should never be negative!";
}

TEST_F(SRP6ServerSessionTest, positive_ephemeral) {
	EXPECT_NO_THROW(session->session_key(1));
}

//todo, needs SRP6 client implementation
TEST_F(SRP6ServerSessionTest, m1_match) {
	EXPECT_EQ(1, 1) << "M1 did not match expected value!";
}

//todo, needs SRP6 client implementation
TEST_F(SRP6ServerSessionTest, m2_match) {
	EXPECT_EQ(1, 1) << "M2 did not match expected value!";
}