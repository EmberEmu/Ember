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
#include "../KeyData.h"
#include <shared/utility/HashDefines.h>
#include <boost/assert.hpp>
#include <botan/bigint.h>
#include <array>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::client {

class ReconnectProof final : public Packet {
	static const std::size_t WIRE_LENGTH = 58;
	State state_ = State::initial;

public:
	ReconnectProof()
		: Packet(Opcode::cmd_auth_reconnect_proof) {}

	std::array<std::uint8_t, 16> salt;
	std::array<std::uint8_t, hash_sizes::sha160> proof;
	std::array<std::uint8_t, hash_sizes::sha160> client_checksum;
	std::uint8_t key_count = 0;
	std::vector<KeyData> keys;

	State read_from_stream(spark::io::pmr::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(state_ == State::initial && stream.size() < WIRE_LENGTH) {
			return State::call_again;
		}

		stream >> opcode;
		stream >> salt;
		stream >> proof;
		stream >> client_checksum;
		stream >> key_count;
		// todo, read key data here

		return (state_ = State::done);
	}

	void write_to_stream(spark::io::pmr::BinaryStream& stream) const override {
		stream << opcode;
		stream << salt;
		stream << proof;
		stream << client_checksum;
		stream << key_count;

		for (auto& key : keys) {
			stream << key.len;
			stream << key.pub_value;
			stream << key.product;
			stream << key.hash;
		}
	}
};

} // client, grunt, ember