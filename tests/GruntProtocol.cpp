/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "GruntPacketDumps.h"
#include <spark/BufferChain.h>
#include <login/grunt/Packets.h>
#include <boost/asio/ip/address.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <tuple>
#include <cstdint>

 /*
 * These tests verify that the serialisation & deserialisation of the Grunt login protocol
 * packets is working as expected. The process is roughly as follows:
 *
 * 1) Each packet as it appears on the wire is stored in a byte array. This byte array is then
 *    written into a buffer chain, to simulate reading a complete packet from a socket.
 * 2) The buffer is then deserialised and each field is checked to ensure it matches the expected
 *    value.
 * 3) The deserialised structure is serialised back into the buffer chain and the bytes are
 *    compared to ensure that the output of the serialisation exactly matches the input of the
 *    deserialisation.
 */

using namespace ember;
using namespace std::string_literals;

TEST(GruntProtocol, ClientLoginChallenge) {
	const std::size_t packet_size = sizeof(client_login_challenge);

	// write the packet bytes into chain
	spark::BufferChain<1024> chain;
	spark::BinaryStream stream(chain);
	chain.write(client_login_challenge, packet_size);

	// deserialise the packet
	auto packet = grunt::client::LoginChallenge();
	packet.read_from_stream(stream);

	// verify the deserialisation results
	GameVersion version { 1, 12, 1, 5875 };
	auto ip = boost::asio::ip::address_v4::from_string("10.0.0.5").to_ulong();

	ASSERT_EQ(0, chain.size()) << "Read length incorrect";
	ASSERT_EQ(version, packet.version)
		<< "Deserialisation failed (field: client version)";
	ASSERT_EQ("CHAOSVEX"s, packet.username)
		<< "Deserialisation failed (field: username)";
	ASSERT_EQ(0, packet.timezone_bias)
		<< "Deserialisation failed (field: timezone bias)";
	ASSERT_EQ(grunt::client::LoginChallenge::Platform::x86, packet.platform)
		<< "Deserialisation failed (field: platform)";
	ASSERT_EQ(grunt::client::LoginChallenge::OperatingSystem::Windows, packet.os)
		<< "Deserialisation failed (field: operating system)";
	ASSERT_EQ(grunt::client::LoginChallenge::PacketMagic::WoW, packet.magic)
		<< "Deserialisation failed (field: magic)";
	ASSERT_EQ(grunt::client::LoginChallenge::ClientLocale::enUS, packet.locale)
		<< "Deserialisation failed (field: locale)";
	ASSERT_EQ(ip, packet.ip)
		<< "Deserialisation failed (field: IP)";
	ASSERT_EQ(3, packet.error)
		<< "Deserialisation failed (field: result)";

	// serialise back to the stream and verify that the output matches the original packet
	packet.write_to_stream(stream);
	ASSERT_EQ(packet_size, chain.size()) << "Write length incorrect";

	char buffer[packet_size];
	chain.read(buffer, chain.size());

	ASSERT_EQ(0, memcmp(buffer, client_login_challenge, packet_size))
		<< "Serialisation failed (input != output)";
}

TEST(GruntProtocol, ClientLoginProof) {
	const std::size_t packet_size = sizeof(client_login_proof);

	// write the packet bytes into chain
	spark::BufferChain<1024> chain;
	spark::BinaryStream stream(chain);
	chain.write(client_login_proof, packet_size);

	// deserialise the packet
	auto packet = grunt::client::LoginProof();
	packet.read_from_stream(stream);

	// verify the deserialisation results
	ASSERT_EQ(0, chain.size()) << "Read length incorrect";
	ASSERT_EQ(0, packet.unknown) << "Deserialisation failed (field: unknown)";

	// serialise back to the stream and verify that the output matches the original packet
	packet.write_to_stream(stream);
	ASSERT_EQ(packet_size, chain.size()) << "Write length incorrect";

	char buffer[packet_size];
	chain.read(buffer, chain.size());

	ASSERT_EQ(0, memcmp(buffer, client_login_proof, packet_size))
		<< "Serialisation failed (input != output)";
}

TEST(GruntProtocol, ClientReconnectProof) {
	const std::size_t packet_size = sizeof(client_reconnect_proof);

	// write the packet bytes into chain
	spark::BufferChain<1024> chain;
	spark::BinaryStream stream(chain);
	chain.write(client_reconnect_proof, packet_size);

	// deserialise the packet
	auto packet = grunt::client::ReconnectProof();
	packet.read_from_stream(stream);

	// verify the deserialisation results
	std::array<Botan::byte, 16> r1_expected = { 0x39, 0xb1, 0xd1, 0xe4, 0x49, 0x13, 0x80, 0x9d,
	                                            0x92, 0x17, 0x56, 0x8e, 0xa5, 0x7b, 0x24, 0x3d };
	std::array<Botan::byte, 20> r2_expected = { 0xc0, 0x4e, 0x35, 0xe6, 0xfe, 0xb2, 0x1a, 0x84,
	                                            0x98, 0xc4, 0x8e, 0x6c, 0x2d, 0x7f, 0x42, 0x56,
	                                            0xb6, 0x4c, 0x98, 0x24 };
	std::array<Botan::byte, 20> r3_expected = { 0x0d, 0xbe, 0x94, 0xaf, 0x7f, 0x46, 0x94, 0xa2,
	                                            0x3f, 0xe7, 0xdf, 0xe5, 0x99, 0xbf, 0xe7, 0x88,
	                                            0x86, 0x02, 0x5f, 0x92 };

	ASSERT_TRUE(chain.empty()) << "Read length incorrect";

	ASSERT_EQ(r1_expected, packet.R1)
		<< "Deserialisation failed (field: R1)";
	ASSERT_EQ(r2_expected, packet.R2)
		<< "Deserialisation failed (field: R2)";
	ASSERT_EQ(r3_expected, packet.R3)
		<< "Deserialisation failed (field: R3)";
	ASSERT_EQ(0, packet.key_count) << "Deserialisation failed (field: key count)";

	// serialise back to the stream and verify that the output matches the original packet
	packet.write_to_stream(stream);
	ASSERT_EQ(packet_size, chain.size()) << "Write length incorrect";

	char buffer[packet_size];
	chain.read(buffer, chain.size());

	ASSERT_EQ(0, memcmp(buffer, client_reconnect_proof, packet_size))
		<< "Serialisation failed (input != output)";
}

TEST(GruntProtocol, ClientRequestRealmList) {
	const std::size_t packet_size = sizeof(request_realm_list);

	// write the packet bytes into chain
	spark::BufferChain<1024> chain;
	spark::BinaryStream stream(chain);
	chain.write(request_realm_list, packet_size);

	// deserialise the packet
	auto packet = grunt::client::RequestRealmList();
	packet.read_from_stream(stream);

	// verify the deserialisation results
	ASSERT_EQ(0, chain.size()) << "Read length incorrect";
	ASSERT_EQ(0, packet.unknown) << "Deserialisation failed (field: unknown)";

	// serialise back to the stream and verify that the output matches the original packet
	packet.write_to_stream(stream);
	ASSERT_EQ(packet_size, chain.size()) << "Write length incorrect";

	char buffer[packet_size];
	chain.read(buffer, chain.size());

	ASSERT_EQ(0, memcmp(buffer, request_realm_list, packet_size))
		<< "Serialisation failed (input != output)";
}

TEST(GruntProtocol, ServerLoginChallenge) {
	const std::size_t packet_size = sizeof(server_login_challenge);

	// write the packet bytes into chain
	spark::BufferChain<1024> chain;
	chain.write(server_login_challenge, packet_size);

	// deserialise the packet
	spark::BinaryStream stream(chain);
	auto packet = grunt::server::LoginChallenge();
	packet.read_from_stream(stream);

	// verify the deserialisation results
	ASSERT_EQ(0, chain.size()) << "Read length incorrect";
	ASSERT_EQ(Botan::BigInt("0x153a794dba6475ef8b2cfdbb8fc88d40edc0effea638842829d4d2c4baba84f1"), packet.B)
		<< "Deserialisation failed (field: B [public ephemeral])";
	ASSERT_EQ(7, packet.g) << "Deserialisation failed (field: g [generator])";
	ASSERT_EQ(1, packet.g_len) << "Deserialisation failed (field: g length)";
	ASSERT_EQ(Botan::BigInt("0x894B645E89E1535BBDAD5B8B290650530801B18EBFBF5E8FAB3C82872A3E9BB7"), packet.N)
		<< "Deserialisation failed (field: N [safe prime])";
	ASSERT_EQ(32, packet.n_len) << "Deserialisation failed (field: N length)";
	ASSERT_EQ(grunt::ResultCode::SUCCESS, packet.result) << "Deserialisation failed (field: result)";
	ASSERT_EQ(Botan::BigInt("0xF4C7DBCA7138DA48D9B7BE55C0C76B1145AF67340CF7A6718D452A563E12A19C"), packet.s)
		<< "Deserialisation failed (field: salt)";
	ASSERT_EQ(0, packet.unk1) << "Deserialisation failed (field: unknown 1)";
	//ASSERT_EQ(0, packet.unk3) << "Deserialisation failed (field: unknown 3)";
	ASSERT_EQ(0, packet.unk4) << "Deserialisation failed (field: unknown 4)";

	// serialise back to the stream and verify that the output matches the original packet
	packet.write_to_stream(stream);
	ASSERT_EQ(packet_size, chain.size()) << "Write length incorrect";

	char buffer[packet_size];
	chain.read(buffer, chain.size());

	ASSERT_EQ(0, memcmp(buffer, server_login_challenge, packet_size))
		<< "Serialisation failed (input != output)";
}

TEST(GruntProtocol, ServerLoginProof) {
	const std::size_t packet_size = sizeof(server_login_proof);

	// write the packet bytes into chain
	spark::BufferChain<1024> chain;
	spark::BinaryStream stream(chain);
	chain.write(server_login_proof, packet_size);

	// deserialise the packet
	auto packet = grunt::server::LoginProof();
	packet.read_from_stream(stream);

	// verify the deserialisation results
	ASSERT_TRUE(chain.empty()) << "Read length incorrect";
	ASSERT_EQ(0, packet.account_flags) << "Deserialisation failed (field: account flags)";
	ASSERT_EQ(Botan::BigInt("0xa3fbd672a092de7650fb0733419ebf59bcae644a"), packet.M2)
		<< "Deserialisation failed (field: M2)";
	ASSERT_EQ(grunt::ResultCode::SUCCESS, packet.result) << "Deserialisation failed (field: result)";
	ASSERT_EQ(0, packet.account_flags) << "Deserialisation failed (field: unknown)";

	// serialise back to the stream and verify that the output matches the original packet
	packet.write_to_stream(stream);
	ASSERT_EQ(packet_size, chain.size()) << "Write length incorrect";

	char buffer[packet_size];
	chain.read(buffer, chain.size());

	ASSERT_EQ(0, memcmp(buffer, server_login_proof, packet_size))
		<< "Serialisation failed (input != output)";
}

TEST(GruntProtocol, ServerRealmList) {
	const std::size_t packet_size = sizeof(realm_list);

	// write the packet bytes into chain
	spark::BufferChain<1024> chain;
	spark::BinaryStream stream(chain);
	chain.write(realm_list, packet_size);

	// deserialise the packet
	auto packet = grunt::server::RealmList();
	packet.read_from_stream(stream);

	// verify the deserialisation results
	ASSERT_EQ(0, chain.size()) << "Read length incorrect";
	ASSERT_EQ(2, packet.realms.size()) << "Deserialisation failed (invalid realm count)";

	// test first realm entry
	grunt::server::RealmList::RealmListEntry entry = packet.realms[0];
	auto realm = entry.realm;
	auto chars = entry.characters;

	ASSERT_EQ(2, realm.flags) << "Deserialisation failed (invalid realm flags)";
	ASSERT_EQ(1, realm.icon) << "Deserialisation failed (invalid realm icon)";
	ASSERT_EQ("127.0.0.1:1337"s, realm.ip) << "Deserialisation failed (invalid realm IP)";
	ASSERT_EQ("Ember"s, realm.name) << "Deserialisation failed (invalid realm name)";
	ASSERT_EQ(0.0f, realm.population) << "Deserialisation failed (invalid realm population)";
	ASSERT_EQ(0, chars) << "Deserialisation failed (invalid character count)";

	//test second realm entry
	entry = packet.realms[1];
	realm = entry.realm;
	chars = entry.characters;

	ASSERT_EQ(1, realm.flags) << "Deserialisation failed (invalid realm flags)";
	ASSERT_EQ(0, realm.icon) << "Deserialisation failed (invalid realm icon)";
	ASSERT_EQ("127.0.0.1:8085"s, realm.ip) << "Deserialisation failed (invalid realm IP)";
	ASSERT_EQ("Ember Test"s, realm.name) << "Deserialisation failed (invalid realm name)";
	ASSERT_EQ(1.4f, realm.population) << "Deserialisation failed (invalid realm population)";
	ASSERT_EQ(0, chars) << "Deserialisation failed (invalid character count)";

	// check the other fields
	ASSERT_EQ(0, packet.unknown) << "Deserialisation failed (field: unknown)";
	ASSERT_EQ(5, packet.unknown2) << "Deserialisation failed (field: unknown)";

	// serialise back to the stream and verify that the output matches the original packet
	packet.write_to_stream(stream);
	ASSERT_EQ(packet_size, chain.size()) << "Write length incorrect";

	char buffer[packet_size];
	chain.read(buffer, chain.size());

	ASSERT_EQ(0, memcmp(buffer, realm_list, packet_size))
		<< "Serialisation failed (input != output)";
}

TEST(GruntProtocol, ServerReconnectChallenge) {
	const std::size_t packet_size = sizeof(server_reconnect_challenge);

	// write the packet bytes into chain
	spark::BufferChain<1024> chain;
	spark::BinaryStream stream(chain);
	chain.write(server_reconnect_challenge, packet_size);

	// deserialise the packet
	auto packet = grunt::server::ReconnectChallenge();
	packet.read_from_stream(stream);

	// verify the deserialisation results
	std::array<Botan::byte, 16> rand{ 0xdd, 0x26, 0xe7, 0x5f, 0x44, 0x24, 0xcf, 0xdd, 0x51, 0x89,
	                                  0x41, 0xd2, 0x02, 0x78, 0xd1, 0x84 };
	ASSERT_EQ(0, chain.size()) << "Read length incorrect";
	ASSERT_EQ(rand, packet.rand) << "Deserialisation failed (field: unknown)";
	ASSERT_EQ(grunt::ResultCode::SUCCESS, packet.result) << "Deserialisation failed (field: unknown)";
	ASSERT_EQ(0, packet.unknown) << "Deserialisation failed (field: unknown)";
	ASSERT_EQ(0, packet.unknown2) << "Deserialisation failed (field: unknown)";

	// serialise back to the stream and verify that the output matches the original packet
	packet.write_to_stream(stream);
	ASSERT_EQ(packet_size, chain.size()) << "Write length incorrect";

	char buffer[packet_size];
	chain.read(buffer, chain.size());

	ASSERT_EQ(0, memcmp(buffer, server_reconnect_challenge, packet_size))
		<< "Serialisation failed (input != output)";
}

TEST(GruntProtocol, ServerReconnectProof) {
	const std::size_t packet_size = sizeof(server_reconnect_proof);

	// write the packet bytes into chain
	spark::BufferChain<1024> chain;
	spark::BinaryStream stream(chain);
	chain.write(server_reconnect_proof, packet_size);

	// deserialise the packet
	auto packet = grunt::server::ReconnectProof();
	packet.read_from_stream(stream);

	// verify the deserialisation results
	ASSERT_EQ(0, chain.size()) << "Read length incorrect";
	ASSERT_EQ(grunt::ResultCode::SUCCESS, packet.result) << "Deserialisation failed (field: result)";

	// serialise back to the stream and verify that the output matches the original packet
	packet.write_to_stream(stream);
	ASSERT_EQ(packet_size, chain.size()) << "Write length incorrect";

	char buffer[packet_size];
	chain.read(buffer, chain.size());

	ASSERT_EQ(0, memcmp(buffer, server_reconnect_proof, packet_size))
		<< "Serialisation failed (input != output)";
}