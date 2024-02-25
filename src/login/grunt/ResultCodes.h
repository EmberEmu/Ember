/*
 * Copyright (c) 2015 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <shared/smartenum.hpp>

namespace ember::grunt {

smart_enum_class(Result, std::uint8_t,
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
	FAIL_OTHER                  = 0XFF
)

} // grunt, ember