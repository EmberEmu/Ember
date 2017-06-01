/*
 * Copyright (c) 2015, 2016 Ember
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
#include <botan/secmem.h>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::server {

class ReconnectChallenge final : public Packet {
	const static std::size_t WIRE_LENGTH = 34;
	const static std::size_t RAND_LENGTH = 16;

	State state_ = State::INITIAL;

public:
	ReconnectChallenge() : Packet(Opcode::CMD_AUTH_RECONNECT_CHALLENGE) {}
	Result result;
	std::array<Botan::byte, RAND_LENGTH> salt;
	std::array<Botan::byte, RAND_LENGTH> checksum_salt; // client no longer uses this

	State read_from_stream(spark::SafeBinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(state_ == State::INITIAL && stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}
		
		stream >> opcode;
		stream >> result;
		stream.get(salt.data(), salt.size());
		stream.get(checksum_salt.data(), checksum_salt.size());

		return (state_ = State::DONE);
	}

	void write_to_stream(spark::BinaryStream& stream) const override {
		stream << opcode;
		stream << result;
		stream.put(salt.data(), salt.size());
		stream.put(checksum_salt.data(), checksum_salt.size());
	}
};

} // server, grunt, ember