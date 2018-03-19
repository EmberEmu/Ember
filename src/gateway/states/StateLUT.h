/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Event.h"
#include <game_protocol/Opcodes.h>
#include <memory>

namespace ember { 

struct ClientContext;

typedef void(*state_func)(ClientContext*);
typedef void(*event_handler)(ClientContext*, const Event*);
typedef void(*packet_handler)(ClientContext*, protocol::ClientOpcode);

extern const state_func enter_states[];
extern const state_func exit_states[];
extern const packet_handler update_packet[];
extern const event_handler update_event[];

} // ember