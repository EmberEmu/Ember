/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientStates.h"
#include "AuthenticationContext.h"
#include "WorldEnterContext.h"
#include "../ClientTimer.h"
#include "../ConnectionDefines.h"
#include "../Forwards.h"
#include <logger/LoggerFwd.h>
#include <shared/utility/UTF8String.h>
#include <optional>
#include <variant>
#include <cstdint>

namespace ember::realm {

struct WorldContext {
	//std::shared_ptr<WorldConnection> world_conn;
};

using StateContext = 
	std::variant<
		authentication::Context,
		world_enter::Context
	>;

struct ClientID {
	std::uint32_t id;
	utf8_string username;
};

struct ClientContext {
	ClientTimer timer;
	ClientHandler& handler;
	ClientConnection* connection;
	const ConfigStore& cfg_store;
	EventDispatcher& dispatcher;
	RealmQueue& queue;
	AccountClient& account_rpc;
	CharacterClient& character_rpc;
	log::Logger& logger;
	BinaryStream* stream;
	ClientState state;
	ClientState prev_state;
	StateContext state_ctx;
	std::optional<ClientID> client_id;
};

} // realm, ember