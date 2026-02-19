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
	success                     = 0x00,
	fail_unknown0               = 0x01, 
	fail_unknown1               = 0x02,
	fail_banned                 = 0x03,
	fail_unknown_account        = 0x04,
	fail_incorrect_password     = 0x05,
	fail_already_online         = 0x06,
	fail_no_time                = 0x07,
	fail_db_busy                = 0x08,
	fail_version_invalid        = 0x09, 
	fail_version_update         = 0x0a,
	fail_invalid_server         = 0x0b,
	fail_suspended              = 0x0c,
	fail_noaccess               = 0x0d,
	success_survey              = 0x0e,
	fail_parental_control       = 0x0f,
	fail_other                  = 0xff
)

} // grunt, ember