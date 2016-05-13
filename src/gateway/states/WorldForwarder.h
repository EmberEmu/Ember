/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <game_protocol/PacketHeaders.h>
#include <spark/Buffer.h>

namespace ember {

class ClientHandler;

class WorldForwarder final {
	ClientHandler& handler_;
	
public:
	WorldForwarder(ClientHandler& handler) : handler_(handler) {}
	void update(protocol::ClientHeader& header, spark::Buffer& buffer);
};

} // ember