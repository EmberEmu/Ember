/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Opcodes.h"
#include "../Packet.h"
#include "../ResultCodes.h"
#include <cstdint>
#include <cstddef>

namespace ember::grunt::server {

class ReconnectProof final : public Packet {
	static const std::size_t wire_length = 2;

	State state_ = State::initial;

public:
	ReconnectProof()
		: Packet(Opcode::cmd_auth_reconnect_proof) {}

	Result result;

	State read_from_stream(PacketStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(state_ == State::initial && stream.size() < wire_length) {
			return State::call_again;
		}

		stream >> opcode;
		stream >> result;
		
		return (state_ = State::done);
	}

	void write_to_stream(PacketStream& stream) const override {
		stream << opcode;
		stream << result;
	}
};

} // server, grunt, ember