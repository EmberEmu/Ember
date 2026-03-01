/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <WorldServiceStub.h>
#include <logger/LoggerFwd.h>

namespace ember::world {

class WorldRPCServer final : public services::WorldService {
	log::Logger& logger_;

	std::optional<rpc::World::StatusT> handle_get_status(
		const rpc::World::RequestStatus& msg,
		const spark::Link& link,
		const spark::Token& token) override;

	std::optional<rpc::World::PlayerEnterResultT> handle_player_enter(
		const rpc::World::PlayerEnter& msg,
		const spark::Link& link,
		const spark::Token& token) override;

	std::optional<rpc::World::PlayerLeaveResultT> handle_player_leave(
		const rpc::World::PlayerLeave& msg,
		const spark::Link& link,
		const spark::Token& token) override;

	void on_link_up(const spark::Link& link) override;
	void on_link_down(const spark::Link& link) override;

public:
	WorldRPCServer(spark::Server& spark, log::Logger& logger);
};

} // world, ember