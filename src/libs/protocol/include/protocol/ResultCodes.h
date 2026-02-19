/*
 * Copyright (c) 2016  - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/smartenum.hpp>
#include <cstdint>

namespace ember::protocol {

smart_enum_class(Result, std::uint8_t,
	response_success                         = 0x00,
	response_failure                         = 0x01,
	response_cancelled                       = 0x02,
	response_disconnected                    = 0x03,
	response_failed_to_connect               = 0x04,
	response_connected                       = 0x05,
	response_version_mismatch                = 0x06,

	cstatus_connecting                       = 0x07,
	cstatus_negotiating_security             = 0x08,
	cstatus_negotiation_complete             = 0x09,
	cstatus_negotiation_failed               = 0x0a,
	cstatus_authenticating                   = 0x0b,

	auth_ok                                  = 0x0c,
	auth_failed                              = 0x0d,
	auth_reject                              = 0x0e,
	auth_bad_server_proof                    = 0x0f,
	auth_unavailable                         = 0x10,
	auth_system_error                        = 0x11,
	auth_billing_error                       = 0x12,
	auth_billing_expired                     = 0x13,
	auth_version_mismatch                    = 0x14,
	auth_unknown_account                     = 0x15,
	auth_incorrect_password                  = 0x16,
	auth_session_expired                     = 0x17,
	auth_server_shutting_down                = 0x18,
	auth_already_logging_in                  = 0x19,
	auth_login_server_not_found              = 0x1a,
	auth_wait_queue                          = 0x1b,
	auth_banned                              = 0x1c,
	auth_already_online                      = 0x1d,
	auth_no_time                             = 0x1e,
	auth_db_busy                             = 0x1f, // Session has timed out
	auth_suspended                           = 0x20,
	auth_parental_control                    = 0x21,

	realm_list_in_progress                   = 0x22,
	realm_list_success                       = 0x23,
	realm_list_failed                        = 0x24,
	realm_list_invalid                       = 0x25,
	realm_list_realm_not_found               = 0x26,

	account_create_in_progress               = 0x27,
	account_create_success                   = 0x28,
	account_create_failed                    = 0x29,

	char_list_retrieving                     = 0x2a,
	char_list_retrieved                      = 0x2b,
	char_list_failed                         = 0x2c,

	char_create_in_progress                  = 0x2d,
	char_create_success                      = 0x2e,
	char_create_error                        = 0x2f,
	char_create_failed                       = 0x30,
	char_create_name_in_use                  = 0x31,
	char_create_disabled                     = 0x32,
	char_create_pvp_teams_violation          = 0x33,
	char_create_server_limit                 = 0x34,
	char_create_account_limit                = 0x35,
	char_create_server_queue                 = 0x36,
	char_create_only_existing                = 0x37,

	char_delete_in_progress                  = 0x38,
	char_delete_success                      = 0x39,
	char_delete_failed                       = 0x3a,
	char_delete_failed_locked_for_transfer   = 0x3b,

	char_login_in_progress                   = 0x3c,
	char_login_success                       = 0x3d,
	char_login_no_world                      = 0x3e,
	char_login_duplicate_character           = 0x3f,
	char_login_no_instances                  = 0x40,
	char_login_failed                        = 0x41,
	char_login_disabled                      = 0x42,
	char_login_no_character                  = 0x43,
	char_login_locked_for_transfer           = 0x44,

	char_name_no_name                        = 0x45,
	char_name_too_short                      = 0x46,
	char_name_too_long                       = 0x47,
	char_name_only_letters                   = 0x48,
	char_name_mixed_languages                = 0x49,
	char_name_profane                        = 0x4a,
	char_name_reserved                       = 0x4b,
	char_name_invalid_apostrophe             = 0x4c,
	char_name_multiple_apostrophes           = 0x4d,
	char_name_three_consecutive              = 0x4e,
	char_name_invalid_space                  = 0x4f,
	char_name_success                        = 0x50,
	char_name_failure                        = 0x51,
)

} // protocol, ember
