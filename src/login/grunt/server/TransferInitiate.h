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
#include <boost/endian/arithmetic.hpp>
#include <shared/utility/HashDefines.h>
#include <gsl/narrow>
#include <cstdint>
#include <cstddef>

namespace ember::grunt::server {

namespace be = boost::endian;

class TransferInitiate final : public Packet {
	static const std::size_t WIRE_LENGTH = 1;
	State state_ = State::initial;

public:
	TransferInitiate()
		: Packet(Opcode::cmd_xfer_initiate) {}
	
	std::string filename;
	be::little_uint64_t filesize = 0;
	std::array<std::uint8_t, hash_sizes::md5> md5;

	State read_from_stream(spark::io::pmr::BinaryStream& stream) override {
		BOOST_ASSERT_MSG(state_ != State::done, "Packet already complete - check your logic!");

		if(stream.size() < WIRE_LENGTH) {
			return State::call_again;
		}

		stream >> opcode;
		return (state_ = State::done);
	}

	void write_to_stream(spark::io::pmr::BinaryStream& stream) const override {
		stream << opcode;
		stream << gsl::narrow<std::uint8_t>(filename.size());
		stream << filename.c_str();
		stream << filesize;
		stream.put(md5);
	}
};

} // client, grunt, ember