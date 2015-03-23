/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LoginPacketCheck.h"
#include "PacketBuffer.h"
#include "Protocol.h"
#include <logger/Logging.h>
#include <stdexcept>
#include <cstddef>

namespace ember { namespace protocol {

namespace {

bool check_challenge_completion(const PacketBuffer& buffer) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	auto data = buffer.data<const protocol::ClientLoginChallenge>();

	// Ensure we've at least read the challenge header before continuing
	if(buffer.size() < sizeof(protocol::ClientLoginChallenge::Header)) {
		return false;
	}

	// Ensure we've read the entire packet
	std::size_t completed_size = data->header.size + sizeof(data->header);

	if(buffer.size() < completed_size) {
		std::size_t remaining = completed_size - buffer.size();

		// Continue reading if there's enough space in the buffer
		if(remaining < buffer.free()) {
			return false;
		}

		// Client claimed the packet is bigger than it ought to be
		throw std::runtime_error("Buffer too small to hold challenge packet");
	}

	// Packet should be complete by this point
	if(buffer.size() < sizeof(protocol::ClientLoginChallenge)
		|| data->username + data->username_len != buffer.data<const char>() + buffer.size()) {
		throw std::runtime_error("Malformed challenge packet");
	}

	return true;
}

} // unnamed

bool check_packet_completion(const PacketBuffer& buffer) {
	LOG_TRACE_GLOB << __func__ << LOG_ASYNC;

	if(buffer.size() < sizeof(protocol::ClientOpcodes)) {
		return false;
	}

	auto opcode = *static_cast<const protocol::ClientOpcodes*>(buffer.data());

	switch(opcode) {
		case protocol::ClientOpcodes::CMSG_LOGIN_CHALLENGE:
		case protocol::ClientOpcodes::CMSG_RECONNECT_CHALLENGE:
			return check_challenge_completion(buffer);
		case protocol::ClientOpcodes::CMSG_LOGIN_PROOF:
			return buffer.size() >= sizeof(protocol::ClientLoginProof);
		case protocol::ClientOpcodes::CMSG_RECONNECT_PROOF:
			return buffer.size() >= sizeof(protocol::ClientReconnectProof);
		case protocol::ClientOpcodes::CMSG_REQUEST_REALM_LIST:
			return buffer.size() >= sizeof(protocol::RequestRealmList);
		default:
			throw std::runtime_error("Unhandled opcode");
	}
}

}} // protocol, ember
