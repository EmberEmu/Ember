/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientStates.h"
#include "AuthenticationContext.h"
#include "WorldEnterContext.h"
#include "../ClientConnection.h"
#include "../ClientLogHelper.h"
#include "../ConnectionDefines.h"
#include "../Forwards.h"
#include <protocol/Concepts.h>
#include <protocol/StreamResult.h>
#include <logger/Logger.h>
#include <shared/utility/UTF8String.h>
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
	BinaryStream* stream_;
	mutable std::string client_id_str_;
	mutable std::string client_id_ext_;
	ClientConnection* connection_;

	void connection(ClientConnection& connection) {
		connection_ = &connection;
	}

	inline void stream(BinaryStream& stream) {
		stream_ = &stream;
	}

public:
	ClientContext(ClientHandler& handler, const ConfigStore& cfg_store, EventDispatcher& dispatcher,
	              RealmQueue& queue, AccountClient& account_rpc, CharacterClient& character_rpc,
	              const RealmService& realm_rpc, log::Logger& logger);

	std::string_view client_identify() const;

	inline BinaryStream& stream() const {
		assert(stream_);
		return *stream_;
	}

	// handler proxies
	void start_timer(std::chrono::milliseconds time);
	void cancel_timer();
	void stream_err(const protocol::StreamResult& result);
	void state_update(ClientState state);

	// connection proxies
	void set_key(std::span<const std::uint8_t> key);
	bool packet_logging();

	inline void send(protocol::is_packet auto& packet) {
		PACKET_TRACE((*this), "<- {}", packet.opcode);
		connection_->send(packet);
	}

	StateContext state_ctx;
	std::optional<ClientID> client_id;
	ClientHandler& handler;
	const ConfigStore& cfg_store;
	EventDispatcher& dispatcher;
	RealmQueue& queue;
	AccountClient& account_rpc;
	CharacterClient& character_rpc;
	const RealmService& realm_rpc;
	log::Logger& logger;

	friend class ClientHandler;
};

} // realm, ember