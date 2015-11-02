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
#include <gtest/gtest.h>
#include <memory>

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

TEST(GruntProtocol, CMSG_LOGIN_CHALLENGE) {

}

TEST(GruntProtocol, CMSG_LOGIN_PROOF) {

}

TEST(GruntProtocol, CMSG_RECONNECT_PROOF) {

}

TEST(GruntProtocol, CMSG_REQUEST_REALM_LIST) {
	const std::size_t packet_size = sizeof(request_realm_list);

	// write the packet bytes into chain
	spark::BufferChain<1024> chain;
	spark::BinaryStream stream(chain);
	chain.write(request_realm_list, packet_size);

	// deserialise the packet
	auto packet = grunt::client::RequestRealmList();
	packet.read_from_stream(stream);

	// verify the deserialisation results
	ASSERT_TRUE(chain.empty()) << "Read too short";
	ASSERT_EQ(0, packet.unknown) << "Deserialisation failed (field: unknown)";

	// serialise back to the stream and verify that the output matches the original packet
	packet.write_to_stream(stream);
	ASSERT_EQ(packet_size, chain.size()) << "Write too short";

	char buffer[packet_size];
	chain.read(buffer, chain.size());

	ASSERT_EQ(0, memcmp(buffer, request_realm_list, packet_size))
		<< "Serialisation failed (input != output)";
}

TEST(GruntProtocol, SMSG_LOGIN_CHALLENGE) {

}

TEST(GruntProtocol, SMSG_LOGIN_PROOF) {

}

TEST(GruntProtocol, SMSG_REALM_LIST) {

}

TEST(GruntProtocol, SMSG_RECONNECT_CHALLENGE) {

}

TEST(GruntProtocol, SMSG_RECONNECT_PROOF) {

}