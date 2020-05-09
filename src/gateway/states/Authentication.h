/*
 * Copyright (c) 2016 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientContext.h"
#include "../Events.h"
#include <chrono>
#include <memory>
#include <vector>

namespace ember::authentication {

using namespace std::chrono_literals;
constexpr std::chrono::seconds AUTH_TIMEOUT = 30s;

void auth_success(ClientContext& ctx, const std::vector<protocol::client::AuthSession::AddonData>& data);

void enter(ClientContext& ctx);
void handle_packet(ClientContext& ctx, protocol::ClientOpcode opcode);
void handle_event(ClientContext& ctx, const Event* event);
void exit(ClientContext& ctx);

} // authentication, ember