/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Opcodes.h"
#include "../Packet.h"
#include "../Exceptions.h"
#include <boost/endian/conversion.hpp>
#include <cstdint>

namespace ember { namespace grunt { namespace client {

class RequestRealmList : public Packet {
public:
	static const std::size_t WIRE_LENGTH = 5;

	Opcode opcode;
	std::uint32_t unknown;

	State read_from_stream(spark::BinaryStream& stream) override {
		stream >> opcode;
		stream >> unknown;
		boost::endian::little_to_native_inplace(unknown);
		return State::DONE;
	}

	void write_to_stream(spark::BinaryStream& stream) {
		stream << opcode;
		stream << boost::endian::little_to_native(unknown);
	}
};

}}} // client, grunt, ember