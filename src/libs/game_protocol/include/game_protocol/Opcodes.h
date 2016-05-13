/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <shared/smartenum.hpp>

namespace ember { namespace protocol {

smart_enum_class(ServerOpcodes, std::uint16_t,
	SMSG_CHAR_CREATE = 0x03A,
	SMSG_CHAR_ENUM = 0x03B,
	SMSG_CHAR_DELETE = 0x03C,
	SMSG_PONG = 0x1DD,
	SMSG_AUTH_CHALLENGE = 0x1EC,
	SMSG_AUTH_RESPONSE = 0x1EE,
	SMSG_LOGIN_VERIFY_WORLD = 566,
	SMSG_ACCOUNT_DATA_TIMES = 521,
	SMSG_UPDATE_ACCOUNT_DATA = 524,
	SMSG_CHARACTER_LOGIN_FAILED = 0x041,
	SMSG_LOGOUT_COMPLETE = 77
)

smart_enum_class(ClientOpcodes, std::uint32_t,
	CMSG_CHAR_CREATE = 0x36,
	CMSG_CHAR_ENUM = 0x037,
	CMSG_CHAR_DELETE = 0x038,
	CMSG_KEEP_ALIVE = 0x406,
	CMSG_PING = 0x1DC,
	CMSG_AUTH_SESSION = 0x1ED,
	CMSG_PLAYER_LOGIN = 61,
	CMSG_UPDATE_ACCOUNT_DATA = 523
)

}} // grunt, ember