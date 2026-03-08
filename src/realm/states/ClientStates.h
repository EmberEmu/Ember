/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/smartenum.hpp>

namespace ember::realm {

smart_enum(ClientState, char,
	cs_authenticating,
	cs_character_list,
	cs_world_enter,
	cs_world_transfer,
	cs_world_forward,
	cs_session_closed
)

constexpr auto states_max = cs_session_closed;
constexpr auto states_num = states_max + 1;

} // realm, ember