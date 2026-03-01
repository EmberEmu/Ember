/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <WorldClientStub.h>
#include <logger/LoggerFwd.h>

namespace ember::gateway {

class WorldRPCClient final : public services::WorldClient {
	log::Logger& logger_;

	void handle_get_status_response(
		const spark::Link& link,
		const rpc::World::Status& msg) override;

	void handle_register_world_response(
		const spark::Link& link,
		const rpc::World::RegisterResult& msg) override;

	void handle_remove_world_response(
		const spark::Link& link,
		const rpc::World::RemoveResult& msg) override;

	void handle_player_enter_response(
		const spark::Link& link,
		const rpc::World::PlayerEnterResult& msg) override;

	void handle_player_leave_response(
		const spark::Link& link,
		const rpc::World::PlayerLeaveResult& msg) override;

	void on_link_up(const spark::Link& link) override;
	void on_link_down(const spark::Link& link) override;

public:
	WorldRPCClient(spark::Server& spark, log::Logger& logger);
};

} // gateway, ember