/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Opcodes.h"
#include "../Packet.h"
#include "../Exceptions.h"
#include <boost/assert.hpp>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::client {

class RequestRealmList final : public Packet {
	static const std::size_t wire_length = 5;
	State state_ = State::initial;

public:
	RequestRealmList() 
		: Packet(Opcode::cmd_realm_list) {}

	std::uint32_t unknown = 0; // hardcoded to zero in public client, probably some kind of filter

	State read_from_stream(PacketStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(stream.size() < wire_length) {
			return State::call_again;
		}

		stream >> opcode;
		stream >> unknown;

		return (state_ = State::done);
	}

	void write_to_stream(PacketStream& stream) const override {
		stream << opcode;
		stream << unknown;
	}
};

} // client, grunt, ember