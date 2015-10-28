/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

namespace ember { namespace protocol {

//The game can't handle any other values
const std::uint8_t PRIME_LENGTH = 32;
const std::uint8_t PUB_KEY_LENGTH = 32;

enum class ServerOpcodes : std::uint8_t {
	SMSG_LOGIN_CHALLENGE,
	SMSG_LOGIN_PROOF,
	SMSG_RECONNECT_CHALLENGE,
	SMSG_RECONNECT_PROOF,
	SMSG_REQUEST_REALM_LIST   = 0x10,
	SMSG_TRANSFER_INITIATE    = 0x30,
	SMSG_TRANSFER_DATA
};

enum class ResultCodes : std::uint8_t {
	SUCCESS                     = 0x00,
	FAIL_UNKNOWN0               = 0x01, 
	FAIL_UNKNOWN1               = 0x02,
	FAIL_BANNED                 = 0x03,
	FAIL_UNKNOWN_ACCOUNT        = 0x04,
	FAIL_INCORRECT_PASSWORD     = 0x05,
	FAIL_ALREADY_ONLINE         = 0x06,
	FAIL_NO_TIME                = 0x07,
	FAIL_DB_BUSY                = 0x08,
	FAIL_VERSION_INVALID        = 0x09, 
	FAIL_VERSION_UPDATE         = 0x0A,
	FAIL_INVALID_SERVER         = 0x0B,
	FAIL_SUSPENDED              = 0x0C,
	FAIL_NOACCESS               = 0x0D,
	SUCCESS_SURVEY              = 0x0E,
	FAIL_PARENTAL_CONTROL       = 0x0F,
};

}} //protocol, ember