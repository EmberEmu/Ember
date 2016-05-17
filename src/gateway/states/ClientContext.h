/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../ClientStates.h"
#include <spark/Buffer.h>
#include <game_protocol/PacketHeaders.h>
#include <string>
#include <cstdint>

namespace ember {

class ClientHandler;
class ClientConnection;

struct ClientContext {
	bool auth_done;
	protocol::ClientHeader* header;
	spark::Buffer* buffer;
	ClientState state;
	ClientHandler* handler;
	ClientConnection* connection;
	std::uint64_t account_id;
	std::string account_name;
	std::uint32_t auth_seed;
	//std::shared_ptr<WorldConnection> world_conn;
};

} // ember