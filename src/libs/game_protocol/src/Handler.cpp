/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <game_protocol/Handler.h>
#include <game_protocol/Opcodes.h>
#include <game_protocol/Packets.h>
#include <spark/Buffer.h>
#include <boost/assert.hpp>

namespace ember { namespace protocol {

bool Handler::handle_new_packet(spark::Buffer& buffer) {
	ClientOpcodes opcode; // temp
	buffer.copy(&opcode, sizeof(opcode));

	switch(opcode) {
		
		default:
			return false;
	}

	state_ = State::READ;
	return true;
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

boost::optional<PacketHandle> Handler::try_deserialise(spark::Buffer& buffer, bool* error) {
	if(state_ == State::NEW_PACKET && buffer.size() > sizeof(ClientOpcodes)) {
		*error = handle_new_packet(buffer);
	} else if(state_ == State::READ) {
		handle_read(buffer);
	}
	
	if(state_ == State::NEW_PACKET) {
		return std::move(curr_packet_);
	} else {
		return boost::optional<PacketHandle>();
	}
}


}} // game_protocol, ember