/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember { namespace grunt { namespace client {

enum class Opcode : std::uint8_t {
	CMSG_LOGIN_CHALLENGE,
	CMSG_LOGIN_PROOF,
	CMSG_RECONNECT_CHALLENGE,
	CMSG_RECONNECT_PROOF,
	CMSG_REQUEST_REALM_LIST = 0x10,
};

}}} // client, grunt, ember