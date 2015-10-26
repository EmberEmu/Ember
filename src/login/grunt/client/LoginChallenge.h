/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Opcodes.h"
#include "../ResultCodes.h"
#include <string>
#include <cstdint>

namespace ember { namespace grunt { namespace client {

class LoginChallenge {
	Opcode opcode;
	ResultCode error;
	std::uint16_t size;
	std::uint8_t game[4];
	std::uint8_t major;
	std::uint8_t minor;
	std::uint8_t patch;
	std::uint16_t build;
	std::uint8_t platform[4];
	std::uint8_t os[4];
	std::uint8_t country[4];
	std::uint32_t timezone_bias;
	std::uint32_t ip;
	std::uint8_t username_len;
	std::string username;
};

}}} // client, grunt, ember