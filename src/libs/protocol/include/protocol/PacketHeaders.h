/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Opcodes.h>
#include <boost/endian/arithmetic.hpp>

namespace ember::protocol {

using SizeType = boost::endian::big_uint16_at;

struct ServerHeader {
	using OpcodeType = ServerOpcode;
	using SizeType = boost::endian::big_uint16_at;

	static constexpr std::size_t WIRE_SIZE =
		sizeof(SizeType) + sizeof(OpcodeType);
};

struct ClientHeader {
	using OpcodeType = ClientOpcode;
	using SizeType = boost::endian::big_uint16_at;

	static constexpr std::size_t WIRE_SIZE =
		sizeof(SizeType) + sizeof(OpcodeType);
};

} // protocol, ember
