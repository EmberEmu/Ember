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
#include "ClientContext.h"
#include "ClientSink.h"
#include "ConnectionDefines.h"
#include "states/ClientStates.h"
#include <logger/LoggerFwd.h>
#include <protocol/client/Ping.h>
#include <protocol/Concepts.h>
#include <protocol/StreamResult.h>
#include <shared/ClientIdent.h>
#include <memory>
#include <string_view>
#include <cstddef>

namespace ember::realm {

using namespace std::chrono_literals;

class ClientConnection;
class ClientContextBuilder;

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
	ClientState state_;

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

	bool validate_ping(const protocol::client::Ping& ping);
	void handle_ping(BinaryStream& stream);
	void handle_timer();
	void state_update(ClientState new_state);
	bool ping_sent_check();
	bool pps_flood_check();
	bool handle_self_event(const Event& event);

	void send(protocol::is_packet auto& packet);
	void skip(BinaryStream& stream);
	void stream_err(const protocol::StreamResult& result);
	void set_connection(ClientConnection& connection);
	void request_stop();

public:
	ClientHandler(const ClientIdent& ident, ClientContext context, log::Logger& logger);
	~ClientHandler();

	void start(ClientConnection& connection);
	void stop();

	const ClientIdent& uuid() const;
	std::string_view client_identify() const;

	void handle_message(BinaryStream& stream);
	void handle_event(const Event& event);

	void log_redirect(LogRedirect::Type type, log::Severity severity);
	void log_redirect_stop();

	ClientState state() const {
		return state_;
	}

	friend class ClientContext;
};

} // realm, ember

#include "ClientHandler.inl"