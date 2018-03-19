/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Event.h"
#include "states/ClientContext.h"
#include <game_protocol/Packet.h>
#include <game_protocol/PacketHeaders.h> // todo, remove
#include <spark/Buffer.h>
#include <logger/Logging.h>
#include <shared/ClientUUID.h>
#include <boost/uuid/uuid.hpp>
#include <memory>

namespace ember {

class ClientConnection;

class ClientHandler final {
	ClientConnection& connection_;
	ClientContext context_;
	const ClientUUID uuid_;
	log::Logger* logger_;

	std::string client_identify();
	void handle_ping(spark::Buffer& buffer);

public:
	ClientHandler(ClientConnection& connection, ClientUUID uuid, log::Logger* logger);

	void state_update(ClientState new_state);
	void packet_skip(spark::Buffer& buffer, protocol::ClientOpcode opcode);
	bool packet_deserialise(protocol::Packet& packet, spark::Buffer& stream);
	void handle_message(spark::Buffer& buffer, protocol::SizeType size);
	void handle_event(const Event* event);
	void handle_event(std::unique_ptr<const Event> event);

	void start();
	void stop();

	const ClientUUID& uuid() const {
		return uuid_;
	}
};

} // ember