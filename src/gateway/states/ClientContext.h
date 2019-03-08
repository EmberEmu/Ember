/*
 * Copyright (c) 2016 - 2019 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientStates.h"
#include <spark/buffers/Buffer.h>
#include <protocol/PacketHeaders.h>
#include <shared/util/UTF8String.h>
#include <cstdint>

namespace ember {

class ClientHandler;
class ClientConnection;

enum class AuthStatus {
	NOT_AUTHED, IN_PROGRESS, SUCCESS, FAILED
};

struct ClientContext {
	spark::Buffer* buffer;
	protocol::SizeType msg_size;
	ClientState state;
	ClientState prev_state;
	ClientHandler* handler;
	ClientConnection* connection;
	std::uint32_t account_id;
	utf8_string account_name;
	std::uint32_t auth_seed;
	//std::shared_ptr<WorldConnection> world_conn;
	AuthStatus auth_status;
};

} // ember