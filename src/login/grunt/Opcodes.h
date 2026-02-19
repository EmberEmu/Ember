/*
 * Copyright (c) 2015 - 2018 Ember
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
	// auth server opcodes
	cmd_auth_logon_challenge      = 0x00,
	cmd_auth_logon_proof          = 0x01,
	cmd_auth_reconnect_challenge  = 0x02,
	cmd_auth_reconnect_proof      = 0x03,
	cmd_survey_result             = 0x04,
	
	// realm listing server opcodes
	cmd_realm_list                = 0x10,

	// patch server opcodes
	cmd_xfer_initiate             = 0x30,
	cmd_xfer_data                 = 0x31,
	cmd_xfer_accept               = 0x32,
	cmd_xfer_resume               = 0x33,
	cmd_xfer_cancel               = 0x34
)

// These seem to be part of a legacy auth protocol or (more likely) were used by internal services
// Included for posterity
enum class ServerLinkOpcode : std::uint8_t {
	cmd_grunt_auth_verify         = 0x02,
	cmd_grunt_conn_ping           = 0x10,
	cmd_grunt_conn_pong           = 0x11,
	cmd_grunt_hello               = 0x20,
	cmd_grunt_provesession        = 0x21,
	cmd_grunt_kick                = 0x24,
	cmd_grunt_cancel_unknown      = 0x26
};

} // grunt, ember
