/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember { 

struct ClientContext;

typedef void(*enter_state)(ClientContext*);
typedef void(*exit_state)(ClientContext*);
typedef void(*update_state)(ClientContext*);

extern const enter_state update_states[];
extern const exit_state exit_states[];
extern const update_state enter_states[];

} // ember