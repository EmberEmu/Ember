/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Events.h"
#include "FilterTypes.h"
#include "ClientConnection.h"
#include "ClientSink.h"
#include "ConnectionDefines.h"
#include "states/ClientContext.h"
#include <logger/LoggerFwd.h>
#include <protocol/client/Ping.h>
#include <protocol/Packet.h>
#include <shared/ClientIdent.h>
#include <spark/buffers/BinaryStream.h>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace ember::realm {

using namespace std::chrono_literals;

class ClientConnection;

class ClientHandler final {
	// client sanity check rules
	constexpr static auto pps_grace = 3u;
	constexpr static auto pps_soft_limit = 33u;
	constexpr static auto pps_hard_limit = 50u;
	constexpr static auto ping_grace = 5u;
	constexpr static auto ping_delta = 30000ms;
	constexpr static auto ping_leeway = 2000ms;

	ClientConnection* connection_;
	ClientContext context_;
	ClientIdent uuid_;
	boost::asio::steady_timer timer_;
	log::Logger& logger_;
	std::shared_ptr<ClientSink> redirect_sink_;

	// client sanity check state
	unsigned int packet_counter_;
	unsigned int pps_violation_;
	std::uint32_t ping_sequence_;
	std::uint32_t prev_ping_sequence_;
	unsigned int ping_violation_;
	std::chrono::milliseconds last_tick_;
	unsigned int timer_events_;

	mutable std::string client_id_;
	mutable std::string client_id_ext_;

	bool validate_ping(const protocol::client::Ping& ping);
	bool check_ping_sent();
	void handle_ping(BinaryStream& stream);
	void handle_timer();
	bool pps_flood_check();
	bool handle_self_event(const Event& event);
	void packet_log_start();

	void start_timer(const std::chrono::milliseconds& time);
	void cancel_timer();

public:
	ClientHandler(std::size_t index, executor executor, const ConfigStore& cfg_store,
	              EventDispatcher& dispatcher, RealmQueue& queue, AccountClient& account_rpc,
	              CharacterClient& character_rpc, log::Logger& logger);
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

	template<is_packet PacketType>
	std::optional<PacketType> deserialise(BinaryStream& stream);

	void send(is_packet auto& packet);
	bool deserialise(is_packet auto& packet, BinaryStream& stream);
	void skip(BinaryStream& stream);

	void state_update(ClientState new_state);
	void handle_message(StaticBuffer& buffer, protocol::SizeType msg_size);
	void handle_event(const Event& event);

	void log_redirect(LogRedirect::Type type, log::Severity severity);
	void log_redirect_stop();

	ClientIdent& uuid() {
		return uuid_;
	}

	const ClientIdent& uuid() const {
		return uuid_;
	}

	ClientState state() const {
		return context_.state;
	}

	friend class ClientTimer;
};

} // realm, ember

#include "ClientHandler.inl"