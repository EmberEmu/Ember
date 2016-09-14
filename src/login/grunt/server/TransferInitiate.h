/*
 * Copyright (c) 2016 Ember
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
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstddef>

namespace ember { namespace grunt { namespace server {

namespace be = boost::endian;

class TransferInitiate final : public Packet {
	static const std::size_t WIRE_LENGTH = 1;
	State state_ = State::INITIAL;

public:
	TransferInitiate() : Packet(Opcode::CMD_XFER_INITIATE) {}
	
	std::string filename;
	std::uint64_t filesize;
	std::array<std::uint8_t, 16> md5;

	State read_from_stream(spark::SafeBinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::DONE, "Packet already complete - check your logic!");

		if(stream.size() < WIRE_LENGTH) {
			return State::CALL_AGAIN;
		}

		stream >> opcode;


		return (state_ = State::DONE);
	}

	void write_to_stream(spark::BinaryStream& stream) const override {
		stream << opcode;
		stream << static_cast<std::uint8_t>(filename.size());
		stream << filename.c_str();
		stream << filesize;
		stream.put(md5.data(), md5.size());
	}
};

}}} // client, grunt, ember