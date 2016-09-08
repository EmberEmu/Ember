/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Event.h"
#include <memory>

namespace ember { 

struct ClientContext;

typedef void(*state_func)(ClientContext*);
typedef void(*event_handler)(ClientContext*, const Event*);

extern const state_func enter_states[];
extern const state_func update_states[];
extern const state_func exit_states[];
extern const event_handler update_event[];

} // ember