/*
 * Copyright (c) 2020 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "StateFwdDecl.h"
#include <protocol/Opcodes.h>

namespace ember::realm::world_enter {

void enter(ClientContext& ctx);
void handle_packet(ClientContext& ctx, protocol::ClientOpcode opcode);
void handle_event(ClientContext& ctx, const Event* event);
void exit(ClientContext& ctx);

} // world_enter, realm, ember