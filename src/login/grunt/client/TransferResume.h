/*
 * Copyright (c) 2016 - 2026 Ember
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

class TransferResume final : public Packet {
	static const std::size_t wire_length = 9;
	State state_ = State::initial;

public:
	TransferResume()
		: Packet(Opcode::cmd_xfer_resume) {}

	std::uint64_t offset = 0;
	
	State read_from_stream(PacketStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(stream.size() < wire_length) {
			return State::call_again;
		}

		stream >> opcode;
		stream >> offset;

		return stream? state_ = State::done : state_ = State::err_stream_err;
	}

	State write_to_stream(PacketStream& stream) const override {
		stream << opcode;
		stream << offset;
		return stream? State::done : State::err_stream_err;
	}
};

} // client, grunt, ember