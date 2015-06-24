/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtest/gtest.h>
#include <srp6/Server.h>
#include <srp6/Client.h>
#include <srp6/Generator.h>
#include <botan/bigint.h>
#include <botan/secmem.h>
#include <memory>

namespace srp = ember::srp6;

class srp6SessionTest : public ::testing::Test {
public:
	virtual void SetUp() {
		identifier = "CHAOSVEX";
		password = "ABC";
		gen = std::make_unique<srp::Generator>(srp::Generator::Group::_256_BIT);
		salt = srp::generate_salt(32);
		verifier = srp::generate_verifier(identifier, password, *gen, salt, srp::Compliance::GAME);
		server = std::make_unique<srp::Server>(*gen, verifier);
		client = std::make_unique<srp::Client>(identifier, password, *gen);
	}

	virtual void TearDown() {}

	std::string identifier, password;
	Botan::BigInt verifier;
	Botan::BigInt salt;
	std::unique_ptr<srp::Generator> gen;
	std::unique_ptr<srp::Server> server;
	std::unique_ptr<srp::Client> client;
};

TEST(srp6a, RFC5054_TestVectors) {
	std::string identifier = "alice";
	std::string password = "password123";
	Botan::BigInt salt("0xBEB25379D1A8581EB5A727673A2441EE");
	srp::Generator gen(srp::Generator::Group::_1024_BIT);
	
	Botan::BigInt expected_k("0x7556AA045AEF2CDD07ABAF0F665C3E818913186F");
	Botan::BigInt k = srp::detail::compute_k(gen.generator(), gen.prime());
	ASSERT_EQ(expected_k, k) << "K was calculated incorrectly!";

	Botan::BigInt expected_x("0x94B7555AABE9127CC58CCF4993DB6CF84D16C124");
	Botan::BigInt x = srp::detail::compute_x(identifier, password, salt, srp::Compliance::RFC5054);
	ASSERT_EQ(expected_x, x) << "x was calculated incorrectly!";

	Botan::BigInt expected_v("0x7E273DE8696FFC4F4E337D05B4B375BEB0DDE1569E8FA00A9886D8129BADA1F1822"
	                         "223CA1A605B530E379BA4729FDC59F105B4787E5186F5C671085A1447B52A48CF1970"
	                         "B4FB6F8400BBF4CEBFBB168152E08AB5EA53D15C1AFF87B2B9DA6E04E058AD51CC72B"
	                         "FC9033B564E26480D78E955A5E29E7AB245DB2BE315E2099AFB");
	Botan::BigInt v = srp::generate_verifier(identifier, password, gen, salt, srp::Compliance::RFC5054);
	ASSERT_EQ(expected_v, v) << "v was calculated incorrectly!";

	Botan::BigInt test_a("0x60975527035CF2AD1989806F0407210BC81EDC04E2762A56AFD529DDDA2D4393");
	Botan::BigInt test_b("0xE487CB59D31AC550471E81F00F6928E01DDA08E974A004F49E61F5D105284D20");

	srp::Client client(identifier, password, gen, test_a, true);
	srp::Server server(gen, v, test_b, true);

	Botan::BigInt expected_A("0x61D5E490F6F1B79547B0704C436F523DD0E560F0C64115BB72557EC4"
	                         "4352E8903211C04692272D8B2D1A5358A2CF1B6E0BFCF99F921530EC"
	                         "8E39356179EAE45E42BA92AEACED825171E1E8B9AF6D9C03E1327F44"
	                         "BE087EF06530E69F66615261EEF54073CA11CF5858F0EDFDFE15EFEA"
	                         "B349EF5D76988A3672FAC47B0769447B");
	ASSERT_EQ(expected_A, client.public_ephemeral())
		<< "Client's public ephemeral did not match expected value!";

	Botan::BigInt expected_B("0xBD0C61512C692C0CB6D041FA01BB152D4916A1E77AF46AE105393011"
	                         "BAF38964DC46A0670DD125B95A981652236F99D9B681CBF87837EC99"
	                         "6C6DA04453728610D0C6DDB58B318885D7D82C7F8DEB75CE7BD4FBAA"
	                         "37089E6F9C6059F388838E7A00030B331EB76840910440B1B27AAEAE"
	                         "EB4012B7D7665238A8E3FB004B117B58");
	ASSERT_EQ(expected_B, server.public_ephemeral())
		<< "Server's public ephemeral did not match expected value!";

	Botan::BigInt expected_u("0xCE38B9593487DA98554ED47D70A7AE5F462EF019");
	Botan::BigInt u = srp::detail::scrambler(expected_A, expected_B, gen.prime().bytes(),
	                                          srp::Compliance::RFC5054);
	ASSERT_EQ(expected_u, u) << "Scrambling parameter did not match";

	Botan::BigInt expected_key("0xB0DC82BABCF30674AE450C0287745E7990A3381F63B387AAF271A10D"
	                           "233861E359B48220F7C4693C9AE12B0A6F67809F0876E2D013800D6C"
	                           "41BB59B6D5979B5C00A172B4A2A5903A0BDCAF8A709585EB2AFAFA8F"
	                           "3499B200210DCC1F10EB33943CD67FC88A2F39A4BE5BEC4EC0A3212D"
	                           "C346D7E474B29EDE8A469FFECA686E5A");
	EXPECT_EQ(expected_key, Botan::BigInt::decode(client.session_key(expected_B, salt, false,
	                                              srp::Compliance::RFC5054)))
		<< "Client key did not match expected value!";
	EXPECT_EQ(expected_key, Botan::BigInt::decode(server.session_key(expected_A, false,
	                                              srp::Compliance::RFC5054)))
		<< "Server key did not match expected value!";
}

TEST_F(srp6SessionTest, SelfAuthentication) {
	Botan::BigInt A = client->public_ephemeral();
	Botan::BigInt B = server->public_ephemeral();

	srp::SessionKey s_key = server->session_key(A);
	srp::SessionKey c_key = client->session_key(B, salt);

	Botan::BigInt c_proof = client->generate_proof(c_key);
	Botan::BigInt s_proof = server->generate_proof(s_key, c_proof);

	Botan::BigInt expected_c_proof = srp::generate_client_proof(identifier, s_key, gen->prime(),
	                                                            gen->generator(), A, B, salt);
	Botan::BigInt expected_s_proof = srp::generate_server_proof(A, c_proof, c_key);

	EXPECT_EQ(expected_c_proof, c_proof) << "Server could not verify client proof!";
	EXPECT_EQ(expected_s_proof, s_proof) << "Client could not verify server proof!";
}

//todo
TEST_F(srp6SessionTest, GameAuthentication) {
	EXPECT_EQ(1, 1) << "Failed to authenticate with game client!";
}

TEST_F(srp6SessionTest, ServerZeroEphemeral) {
	EXPECT_THROW(server->session_key(0), srp::exception)
		<< "Public ephemeral key should never be zero!";
}

TEST_F(srp6SessionTest, ServerNegativeEphemeral) {
	EXPECT_THROW(server->session_key(Botan::BigInt("-10")), srp::exception)
		<< "Public ephemeral key should never be negative!";
}

TEST_F(srp6SessionTest, ClientZeroEphemeral) {
	EXPECT_THROW(client->session_key(0, salt), srp::exception)
		<< "Public ephemeral key should never be zero!";
}

TEST_F(srp6SessionTest, ClientNegativeEphemeral) {
	EXPECT_THROW(client->session_key(Botan::BigInt("-10"), salt), srp::exception)
		<< "Public ephemeral key should never be negative!";
}