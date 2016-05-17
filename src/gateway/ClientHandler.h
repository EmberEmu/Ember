/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "states/ClientContext.h"
#include <game_protocol/Packet.h>
#include <game_protocol/PacketHeaders.h> // todo, remove
#include <spark/Buffer.h>
#include <logger/Logging.h>

namespace ember {

class ClientConnection;

class ClientHandler final {
	ClientConnection& connection_;
	ClientContext context_;
	protocol::ClientHeader* header_;
	log::Logger* logger_;

	void handle_ping(spark::Buffer& buffer);

public:
	explicit ClientHandler(ClientConnection& connection, log::Logger* logger);
	~ClientHandler();

	bool packet_deserialise(protocol::Packet& packet, spark::Buffer& stream);
	void handle_packet(protocol::ClientHeader header, spark::Buffer& buffer);
	void start();
};

} // ember