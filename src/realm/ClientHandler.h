/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Event.h"
#include "FilterTypes.h"
#include "ConnectionDefines.h"
#include "states/ClientContext.h"
#include <logger/LoggerFwd.h>
#include <protocol/Packet.h>
#include <spark/buffers/BinaryStream.h>
#include <shared/ClientIdent.h>
#include <boost/asio/steady_timer.hpp>
#include <boost/uuid/uuid.hpp>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace ember::realm {

class ClientConnection;

class ClientHandler final {
	constexpr static auto pps_grace = 3u;
	constexpr static auto pps_soft_limit = 1000u;
	constexpr static auto pps_hard_limit = 1500u;

	ClientConnection* connection_;
	ClientContext context_;
	const ClientIdent uuid_;
	boost::asio::steady_timer timer_;
	log::Logger& logger_;
	unsigned int packets_;
	unsigned int pps_violation_;

	mutable std::string client_id_;
	mutable std::string client_id_ext_;

	void handle_ping(BinaryStream& stream);
	void handle_timer();
	void pps_rate_limit();

public:
	ClientHandler(ClientIdent uuid, executor executor, log::Logger& logger);
	~ClientHandler();

	void set_connection(ClientConnection* connection) {
		assert(connection);
		connection_ = connection;
		context_.connection = connection;
	}

	void start();
	void stop();
	void close();
	std::string_view client_identify() const;

	template<is_packet T>
	std::optional<T> deserialise(BinaryStream& stream);

	bool deserialise(is_packet auto& packet, BinaryStream& stream);
	void skip(BinaryStream& stream);

	void state_update(ClientState new_state);
	void handle_message(StaticBuffer& buffer, protocol::SizeType msg_size);
	void handle_event(const Event* event);
	void handle_event(std::unique_ptr<const Event> event);

	void start_timer(const std::chrono::milliseconds& time);
	void cancel_timer();

	const ClientIdent& uuid() const {
		return uuid_;
	}
};

} // realm, ember

#include "ClientHandler.inl"