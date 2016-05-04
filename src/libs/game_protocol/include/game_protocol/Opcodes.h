/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember { namespace protocol {

enum class ServerOpcodes : std::uint16_t { // todo, temp
	SMSG_AUTH_CHALLENGE = 0x1EC,
	SMSG_AUTH_RESPONSE = 0x1EE
};

enum class ClientOpcodes : std::uint32_t { // todo, temp
	CMSG_AUTH_SESSION = 0x1ED
};

}} // grunt, ember