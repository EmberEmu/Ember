/*
 * Copyright (c) 2015 - 2020 Ember
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
#include <boost/endian/arithmetic.hpp>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::client {

namespace be = boost::endian;

class RequestRealmList final : public Packet {
	static const std::size_t WIRE_LENGTH = 5;
	State state_ = State::initial;

public:
	RequestRealmList() 
		: Packet(Opcode::cmd_realm_list) {}

	be::little_uint32_t unknown = 0; // hardcoded to zero in public client, probably some kind of filter

	State read_from_stream(spark::io::pmr::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(stream.size() < WIRE_LENGTH) {
			return State::call_again;
		}

		stream >> opcode;
		stream >> unknown;

		return (state_ = State::done);
	}

	void write_to_stream(spark::io::pmr::BinaryStream& stream) const override {
		stream << opcode;
		stream << unknown;
	}
};

} // client, grunt, ember