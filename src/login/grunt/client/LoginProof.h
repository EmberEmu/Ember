/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Opcodes.h"
#include <cstdint>

namespace ember { namespace grunt { namespace client {

class LoginProof {
	Opcode opcode;
	std::uint8_t A[32];
	std::uint8_t M1[20];
	std::uint8_t crc_hash[20];
	std::uint8_t key_count;
	std::uint8_t unknown;

public:

};

}}} // client, grunt, ember