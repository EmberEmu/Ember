/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Opcodes.h>
#include <boost/endian/arithmetic.hpp>

namespace ember::protocol {

using SizeType = typename boost::endian::big_uint16_at;

struct ServerHeader {
	static constexpr std::size_t WIRE_SIZE =
		sizeof(SizeType) + sizeof(ServerOpcode);

	SizeType size;
	ServerOpcode opcode;
};

struct ClientHeader {
	static constexpr std::size_t WIRE_SIZE =
		sizeof(SizeType) + sizeof(ClientOpcode);

	SizeType size;
	ClientOpcode opcode;
};

} // protocol, ember
