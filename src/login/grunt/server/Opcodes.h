/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember { namespace grunt { namespace server {

enum class Opcode : std::uint8_t {
	SMSG_LOGIN_CHALLENGE,
	SMSG_LOGIN_PROOF,
	SMSG_RECONNECT_CHALLENGE,
	SMSG_RECONNECT_PROOF,
	SMSG_REQUEST_REALM_LIST   = 0x10,
	SMSG_TRANSFER_INITIATE    = 0x30,
	SMSG_TRANSFER_DATA
};

}}} // server, grunt, ember