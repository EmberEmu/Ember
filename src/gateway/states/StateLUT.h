/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Event.h"
#include <protocol/Opcodes.h>
#include <memory>

namespace ember { 

struct ClientContext;

using state_func = void(*)(ClientContext&);
using event_handler = void(*)(ClientContext&, const Event*);
using packet_handler = void(*)(ClientContext&, protocol::ClientOpcode);

extern const state_func enter_states[];
extern const state_func exit_states[];
extern const packet_handler update_packet[];
extern const event_handler update_event[];

} // ember