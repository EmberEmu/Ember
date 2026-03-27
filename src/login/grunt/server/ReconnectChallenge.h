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
#include "../ResultCodes.h"
#include <botan/bigint.h>
#include <botan/auto_rng.h>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::server {

class ReconnectChallenge final : public Packet {
	const static std::size_t wire_length = 34;
	const static std::size_t rand_length = 16;

	State state_ = State::initial;

public:
	ReconnectChallenge()
		: Packet(Opcode::cmd_auth_reconnect_challenge) {}

	Result result;
	std::array<std::uint8_t, rand_length> salt;
	std::array<std::uint8_t, rand_length> checksum_salt; // client no longer uses this

	State read_from_stream(PacketStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(state_ == State::initial && stream.size() < wire_length) {
			return State::call_again;
		}
		
		stream >> opcode;
		stream >> result;
		stream >> salt;
		stream >> checksum_salt;

		return (state_ = State::done);
	}

	void write_to_stream(PacketStream& stream) const override {
		stream << opcode;
		stream << result;
		stream << salt;
		stream << checksum_salt;
	}
};

} // server, grunt, ember