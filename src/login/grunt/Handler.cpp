/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Handler.h"
#include "Packets.h"
#include <spark/Buffer.h>

namespace ember { namespace grunt {

void Handler::handle_new_packet(spark::Buffer& buffer) {
	client::Opcode opcode;
	buffer.copy(&opcode, sizeof(opcode));

	switch(opcode) {
		case client::Opcode::CMSG_LOGIN_CHALLENGE:
		case client::Opcode::CMSG_RECONNECT_CHALLENGE:
			curr_packet_ = std::make_unique<client::LoginChallenge>();
			wire_length = client::LoginChallenge::WIRE_LENGTH;
			break;
		case client::Opcode::CMSG_LOGIN_PROOF:
			curr_packet_ = std::make_unique<client::LoginProof>();
			wire_length = client::LoginProof::WIRE_LENGTH;
			break;
		case client::Opcode::CMSG_RECONNECT_PROOF:
			curr_packet_ = std::make_unique<client::ReconnectProof>();
			wire_length = client::ReconnectProof::WIRE_LENGTH;
			break;
		case client::Opcode::CMSG_REQUEST_REALM_LIST:
			curr_packet_ = std::make_unique<client::RequestRealmList>();
			wire_length = client::RequestRealmList::WIRE_LENGTH;
			break;
		default:
			throw bad_packet("Unknown opcode encountered!");
	}

	state_ = State::INITIAL_READ;
}

void Handler::handle_continuation(spark::Buffer& buffer) {
	spark::BinaryStream stream(buffer);
	Packet::State state = curr_packet_->read_from_stream(stream);

	if((state_ == State::INITIAL_READ && buffer.size() >= wire_length)
		|| state_ == State::CONTINUATION) {
		state = curr_packet_->read_from_stream(stream);
	}

	if(state == Packet::State::DONE) {
		state_ = State::NEW_PACKET;
	} else if(state == Packet::State::CALL_AGAIN) {
		state_ = State::CONTINUATION;
	} else {
		throw bad_packet("Bad packet state!"); // todo, assert
	}
}

boost::optional<PacketHandle> Handler::try_deserialise(spark::Buffer& buffer) {
	switch(state_) {
		case State::NEW_PACKET:
			handle_new_packet(buffer);
		case State::CONTINUATION:
			handle_continuation(buffer);
			break;
	}

	if(state_ == State::NEW_PACKET) {
		return std::move(curr_packet_);
	} else {
		return boost::optional<PacketHandle>();
	}
}


}} // grunt, ember