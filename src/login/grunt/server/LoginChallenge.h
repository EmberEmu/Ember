/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../ResultCodes.h"
#include "Opcodes.h"
#include <cstdint>

namespace ember { namespace grunt { namespace server {

class ServerLoginChallenge {
	Opcode opcode;
	ResultCode error;
	std::uint8_t unk2;
	std::uint8_t B[32];
	std::uint8_t g_len;
	std::uint8_t g;
	std::uint8_t n_len;
	std::uint8_t N[32];
	std::uint8_t s[32];
	std::uint8_t unk3[16];
	std::uint8_t unk4;
};

}}} // server, grunt, ember