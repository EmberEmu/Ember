/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "StateFwdDecl.h"
#include "../ClientContext.h"
#include <cassert>

namespace ember::realm::world_transfer {

void enter(ClientContext& ctx) {
	assert(false && "Unused state");
}

void handle_packet(ClientContext& ctx, protocol::ClientOpcode opcode) {
	assert(false && "Unused state");
}

void handle_event(ClientContext& ctx, const Event& event) {
	assert(false && "Unused state");
}

void exit(ClientContext& ctx) {
	assert(false && "Unused state");
}

} // world_transfer, realm, ember