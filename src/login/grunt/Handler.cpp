/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Handler.h"
#include "Packets.h"
#include <spark/Buffer.h>
#include <boost/assert.hpp>

namespace ember { namespace grunt {

void Handler::handle_new_packet(spark::Buffer& buffer) {
	Opcode opcode;
	buffer.copy(&opcode, sizeof(opcode));

	switch(opcode) {
		case Opcode::CMD_AUTH_LOGIN_CHALLENGE:
			[[fallthrough]];
		case Opcode::CMD_AUTH_RECONNECT_CHALLENGE:
			curr_packet_ = std::make_unique<client::LoginChallenge>();
			break;
		case Opcode::CMD_AUTH_LOGON_PROOF:
			curr_packet_ = std::make_unique<client::LoginProof>();
			break;
		case Opcode::CMD_AUTH_RECONNECT_PROOF:
			curr_packet_ = std::make_unique<client::ReconnectProof>();
			break;
		case Opcode::CMD_REALM_LIST:
			curr_packet_ = std::make_unique<client::RequestRealmList>();
			break;
		default:
			throw bad_packet("Unknown opcode encountered!");
	}

	state_ = State::READ;
}

void Handler::handle_read(spark::Buffer& buffer) {
	spark::BinaryStream stream(buffer);
	Packet::State state = curr_packet_->read_from_stream(stream);

	switch(state) {
		case Packet::State::DONE:
			state_ = State::NEW_PACKET;
			break;
		case Packet::State::CALL_AGAIN:
			state_ = State::READ;
			break;
		default:
			BOOST_ASSERT_MSG(false, "Unreachable condition hit!");
	}
}

boost::optional<PacketHandle> Handler::try_deserialise(spark::Buffer& buffer) {
	switch(state_) {
		case State::NEW_PACKET:
			handle_new_packet(buffer);
			[[fallthrough]];
		case State::READ:
			handle_read(buffer);
			break;
	}

	if(state_ == State::NEW_PACKET) {
		return std::move(curr_packet_);
	} else {
		return boost::optional<PacketHandle>();
	}
}


}} // grunt, ember