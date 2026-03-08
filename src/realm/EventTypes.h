/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::realm {

enum class EventType {
	queue_success,
    queue_update_position,
	account_id_response,
	session_key_response,
	char_create_response,
	char_delete_response,
	char_enum_response,
	char_rename_response,
	player_login,
	timer_expired
};

} // realm, ember