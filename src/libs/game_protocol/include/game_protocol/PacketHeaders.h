/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/Opcodes.h>
#include <boost/endian/arithmetic.hpp>

namespace ember::protocol {

struct ServerHeader {
	boost::endian::big_uint16_t size;
	ServerOpcodes opcode;
};

struct ClientHeader {
	boost::endian::big_uint16_t size;
	ClientOpcodes opcode;
};

} // protocol, ember
