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
	const static std::size_t WIRE_LENGTH = 34;
	const static std::size_t RAND_LENGTH = 16;

	State state_ = State::initial;

public:
	ReconnectChallenge()
		: Packet(Opcode::cmd_auth_reconnect_challenge) {}

	Result result;
	std::array<std::uint8_t, RAND_LENGTH> salt;
	std::array<std::uint8_t, RAND_LENGTH> checksum_salt; // client no longer uses this

	State read_from_stream(spark::io::pmr::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(state_ == State::initial && stream.size() < WIRE_LENGTH) {
			return State::call_again;
		}
		
		stream >> opcode;
		stream >> result;
		stream.get(salt);
		stream.get(checksum_salt);

		return (state_ = State::done);
	}

	void write_to_stream(spark::io::pmr::BinaryStream& stream) const override {
		stream << opcode;
		stream << result;
		stream.put(salt);
		stream.put(checksum_salt);
	}
};

} // server, grunt, ember