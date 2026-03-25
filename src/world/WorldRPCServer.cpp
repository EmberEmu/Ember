/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WorldRPCServer.h"
#include <logger/Logger.h>

namespace ember::world {

using namespace rpc::World;
using namespace spark;

WorldRPCServer::WorldRPCServer(spark::Server& spark, log::Logger& logger)
	: WorldService(spark)
	, logger_(logger) {
}

void WorldRPCServer::on_link_up(const spark::Link& link) {
	LOG_DEBUG(logger_, "Link up: {}", link.peer_banner);
}

void WorldRPCServer::on_link_down(const spark::Link& link) {
	LOG_DEBUG(logger_, "Link down: {}", link.peer_banner);
}

std::optional<StatusT>
WorldRPCServer::handle_get_status(const RequestStatus& msg, const Link& link, const Token& token) {
	return std::nullopt;
}

std::optional<PlayerEnterResultT>
WorldRPCServer::handle_player_enter(const PlayerEnter& msg, const Link& link, const Token& token) {
	return std::nullopt;
}

std::optional<PlayerLeaveResultT>
WorldRPCServer::handle_player_leave(const PlayerLeave& msg, const Link& link, const Token& token) {
	return std::nullopt;
}

} // world, ember