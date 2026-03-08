/*
 * Copyright (c) 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <protocol/Opcodes.h>
#include <unordered_map>

namespace ember::realm {

enum class Route {
    invalid,
    self,
    social,
    transactor,
    world
};

// todo, autogenerate in packet compiler
const std::unordered_map<protocol::ClientOpcode, Route> cmsg_routes {
    { protocol::ClientOpcode::cmsg_ping, Route::self }
};

} // realm, ember