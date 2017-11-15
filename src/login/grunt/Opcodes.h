/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/smartenum.hpp>
#include <cstdint>

namespace ember::grunt {

smart_enum_class(Opcode, std::uint8_t,
	// Auth server opcodes
	CMD_AUTH_LOGON_CHALLENGE      = 0x00,
	CMD_AUTH_LOGON_PROOF          = 0x01,
	CMD_AUTH_RECONNECT_CHALLENGE  = 0x02,
	CMD_AUTH_RECONNECT_PROOF      = 0x03,
	CMD_SURVEY_RESULT             = 0x04,
	
	// Realm listing server opcodes
	CMD_REALM_LIST                = 0x10,

	// Patch server opcodes
	CMD_XFER_INITIATE             = 0x30,
	CMD_XFER_DATA                 = 0x31,
	CMD_XFER_ACCEPT               = 0x32,
	CMD_XFER_RESUME               = 0x33,
	CMD_XFER_CANCEL               = 0x34
)

// These seem to be part of a legacy auth protocol or (more likely) were used by internal services
// Included for posterity
enum class ServerLinkOpcodes : std::uint8_t {
	CMD_GRUNT_AUTH_VERIFY         = 0x02,
	CMD_GRUNT_CONN_PING           = 0x10,
	CMD_GRUNT_CONN_PONG           = 0x11,
	CMD_GRUNT_HELLO               = 0x20,
	CMD_GRUNT_PROVESESSION        = 0x21,
	CMD_GRUNT_KICK                = 0x24,
	CMD_GRUNT_CANCEL_UNKNOWN      = 0x26
};

} // grunt, ember
