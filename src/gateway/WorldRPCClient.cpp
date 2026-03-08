/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WorldRPCClient.h"
#include <logger/Logger.h>

namespace ember::gateway {

using namespace spark;
using namespace rpc::World;

WorldRPCClient::WorldRPCClient(spark::Server& spark, log::Logger& logger)
	: WorldClient(spark)
	, logger_(logger) {
	connect("127.0.0.1", 6005);
}

void WorldRPCClient::connect_failed(const std::string_view ip, std::uint16_t port) {
	LOG_INFO_ASYNC(logger_, "Failed to connect to world service on {}:{}", ip, port);
}

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
	LOG_DEBUG_ASYNC(logger_, "Link up: {}", link.peer_banner);
}

void WorldRPCClient::on_link_down(const Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link down: {}", link.peer_banner);
}

} // gateway, ember