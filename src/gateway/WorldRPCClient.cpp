/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "WorldRPCClient.h"
#include <logger/Logger.h>

namespace ember::gateway {

using namespace spark;
using namespace rpc::World;

WorldRPCClient::WorldRPCClient(spark::Server& spark, log::Logger& logger)
	: WorldClient(spark)
	, logger_(logger) {}

void WorldRPCClient::handle_get_status_response(const Link& link, const Status& msg) {

}

void WorldRPCClient::handle_register_world_response(const Link& link, const RegisterResult& msg) {

}

void WorldRPCClient::handle_remove_world_response(const Link& link, const RemoveResult& msg) {

}

void WorldRPCClient::handle_player_enter_response(const Link& link, const PlayerEnterResult& msg) {

}

void WorldRPCClient::handle_player_leave_response(const Link& link, const PlayerLeaveResult& msg) {

}

void WorldRPCClient::on_link_up(const Link& link) {

}

void WorldRPCClient::on_link_down(const Link& link) {

}

} // gateway, ember