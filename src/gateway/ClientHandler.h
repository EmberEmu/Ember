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
#include <protocol/Packets.h>
#include <spark/buffers/Buffer.h>
#include <spark/buffers/BinaryStream.h>
#include <logger/Logging.h>
#include <shared/ClientUUID.h>
#include <boost/asio/steady_timer.hpp>
#include <boost/uuid/uuid.hpp>
#include <chrono>
#include <memory>

namespace ember {

class ClientConnection;

class ClientHandler final {
	ClientConnection& connection_;
	ClientContext context_;
	const ClientUUID uuid_;
	log::Logger* logger_;
	boost::asio::steady_timer timer_;

	std::string client_identify();
	void handle_ping(spark::Buffer& buffer);

public:
	ClientHandler(ClientConnection& connection, ClientUUID uuid, log::Logger* logger,
	              boost::asio::io_service& service);

	void start();
	void stop();
	void close();

	template<typename PacketT>
	bool packet_deserialise(PacketT& packet, spark::Buffer& buffer);
	void packet_skip(spark::Buffer& buffer, protocol::ClientOpcode opcode);

	void state_update(ClientState new_state);
	void handle_message(spark::Buffer& buffer, protocol::SizeType size);
	void handle_event(const Event* event);
	void handle_event(std::unique_ptr<const Event> event);

	void start_timer(const std::chrono::milliseconds& time);
	void stop_timer();

	const ClientUUID& uuid() const {
		return uuid_;
	}
};

#include "ClientHandler.inl"

} // ember