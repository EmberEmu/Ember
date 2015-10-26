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

class ReconnectProof {
	Opcode opcode;
	std::uint8_t R1[16];
	std::uint8_t R2[20];
	std::uint8_t R3[20];
	std::uint8_t key_count;

public:

};

}}} // client, grunt, ember