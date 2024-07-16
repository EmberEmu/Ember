/*
 * Copyright (c) 2016 - 2020 Ember
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

class TransferResume final : public Packet {
	static const std::size_t WIRE_LENGTH = 9;
	State state_ = State::INITIAL;

public:
	TransferResume() : Packet(Opcode::CMD_XFER_RESUME) {}

	be::little_uint64_t offset = 0;
	
	State read_from_stream(spark::io::pmr::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		stream >> opcode;
		stream >> offset;

		return (state_ = State::DONE);
	}

	void write_to_stream(spark::io::pmr::BinaryStream& stream) const override {
		stream << opcode;
		stream << offset;
	}
};

} // client, grunt, ember