/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtest/gtest.h>
#include <login/srp6/SRP6Server.h>
#include <login/srp6/SRP6Client.h>
#include <botan/bigint.h>
#include <memory>
#include <iostream>

class SRP6SessionTest : public ::testing::Test {
public:
	virtual void SetUp() {
		identifier = "CHAOSVEX";
		password = "ABC";
		SRP6::Generator gen(g, N);
		sv_pair = SRP6::SVPair(SRP6::generate_verifier(identifier, password, gen, 32));
		server = std::make_unique<SRP6::Server>(gen, sv_pair.verifier);
		client = std::make_unique<SRP6::Client>(identifier, password, gen);
	}

	virtual void TearDown() {}

	std::string identifier, password;
	Botan::BigInt N{"0x00c037c37588b4329887e61c2da3324b1ba4b81a63f9748fed2d8a410c2fc21b1232f0d3bfa024276cfd88448197aae486a63bfca7b8bf7754dfb327c7201f6fd17fd7fd74158bd31ce772c9f5f8ab584548a99a759b5a2c0532162b7b6218e8f142bce2c30d7784689a483e095e701618437913a8c39c3dd0d4ca3c500b885fe3"};
	Botan::BigInt g{2};
	SRP6::SVPair sv_pair;
	std::unique_ptr<SRP6::Server> server;
	std::unique_ptr<SRP6::Client> client;
};

TEST_F(SRP6SessionTest, Authentication) {
	Botan::BigInt A = client->public_ephemeral();
	Botan::BigInt B = server->public_ephemeral();

	SRP6::SessionKey s_key = server->session_key(A);
	SRP6::SessionKey c_key = client->session_key(B, sv_pair.salt);

	Botan::BigInt c_proof = client->generate_proof(c_key);
	Botan::BigInt s_proof = server->generate_proof(s_key, c_proof);

	Botan::BigInt expected_c_proof = SRP6::generate_client_proof(identifier, s_key, N, g, A, B, sv_pair.salt);
	Botan::BigInt expected_s_proof = SRP6::generate_server_proof(A, c_proof, c_key);

	EXPECT_EQ(expected_c_proof, c_proof) << "Server could not verify client proof!";
	EXPECT_EQ(expected_s_proof, s_proof) << "Client could not verify server proof!";
}

TEST_F(SRP6SessionTest, ServerZeroEphemeral) {
	EXPECT_THROW(server->session_key(0), SRP6::exception) << "Public ephemeral key should never be zero!";
}

TEST_F(SRP6SessionTest, ServerNegativeEphemeral) {
	EXPECT_THROW(server->session_key(Botan::BigInt("-10")), SRP6::exception)
		<< "Public ephemeral key should never be negative!";
}

TEST_F(SRP6SessionTest, ClientZeroEphemeral) {
	EXPECT_THROW(client->session_key(0, sv_pair.salt), SRP6::exception) << "Public ephemeral key should never be zero!";
}

TEST_F(SRP6SessionTest, ClientNegativeEphemeral) {
	EXPECT_THROW(client->session_key(Botan::BigInt("-10"), sv_pair.salt), SRP6::exception)
		<< "Public ephemeral key should never be negative!";
}
