/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "states/ClientStates.h"
#include "states/AuthenticationContext.h"
#include "states/WorldEnterContext.h"
#include "ClientConnection.h"
#include "ClientLogHelper.h"
#include "ConnectionDefines.h"
#include "Forwards.h"
#include <protocol/Concepts.h>
#include <protocol/StreamResult.h>
#include <logger/Logger.h>
#include <shared/utility/UTF8String.h>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <string>
#include <span>
#include <string_view>
#include <optional>
#include <variant>
#include <cassert>
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

class ClientContext {
	BinaryStream* stream_ = nullptr;
	ClientConnection* connection_ = nullptr;
	ClientHandler* handler_ = nullptr;

	boost::asio::steady_timer timer_;
	mutable std::string client_id_str_;
	mutable std::string client_id_ext_;

	void connection(ClientConnection& connection) {
		connection_ = &connection;
	}

	inline void stream(BinaryStream& stream) {
		stream_ = &stream;
	}

	void set_handler(ClientHandler& handler) {
		handler_ = &handler;
	}

public:
	ClientContext(executor& executor, const ConfigStore& cfg_store, EventDispatcher& dispatcher,
	              RealmQueue& queue, AccountClient& account_rpc, CharacterClient& character_rpc,
	              const RealmService& realm_rpc, log::Logger& logger);

	std::string_view client_identify() const;

	inline BinaryStream& stream() const {
		assert(stream_);
		return *stream_;
	}

	ClientHandler& handler() const {
		assert(handler_);
		return *handler_;
	}

	void start_timer(const std::chrono::milliseconds& time);
	void cancel_timer();

	// handler proxies
	void stream_err(const protocol::StreamResult& result);
	void state_update(ClientState state);

	// connection proxies
	void set_key(std::span<const std::uint8_t> key);
	bool packet_logging() const;

	inline void send(protocol::is_packet auto& packet) {
		PACKET_TRACE((*this), "<- {}", packet.opcode);
		connection_->send(packet);
	}

	StateContext state_ctx;
	std::optional<ClientID> client_id;
	const ConfigStore& cfg_store;
	EventDispatcher& dispatcher; // this won't be needed once RPCs use coroutines
	RealmQueue& queue;
	AccountClient& account_rpc;
	CharacterClient& character_rpc;
	const RealmService& realm_rpc;
	log::Logger& logger;

	friend class ClientHandler;
};

} // realm, ember