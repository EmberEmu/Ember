/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember { namespace grunt {

enum class Opcode : std::uint8_t {
	CMD_AUTH_LOGIN_CHALLENGE,
	CMD_AUTH_LOGON_PROOF,
	CMD_AUTH_RECONNECT_CHALLENGE,
	CMD_AUTH_RECONNECT_PROOF,
	CMD_REALM_LIST                = 0x10,
	CMD_XFER_INITIATE             = 0x30,
	CMD_XFER_DATA
};

}} // grunt, ember