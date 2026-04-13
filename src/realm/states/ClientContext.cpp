/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "../ClientHandler.h"
#include <format>

namespace ember::realm {

ClientContext::ClientContext(ClientHandler& handler, const ConfigStore& cfg_store, EventDispatcher& dispatcher,
                             RealmQueue& queue, AccountClient& account_rpc, CharacterClient& character_rpc,
                             const RealmService& realm_rpc, log::Logger& logger)
	: stream_(nullptr)
	, connection_(nullptr)
	, handler(handler)
	, cfg_store(cfg_store)
	, dispatcher(dispatcher)
	, queue(queue)
	, account_rpc(account_rpc)
	, character_rpc(character_rpc)
	, realm_rpc(realm_rpc)
	, logger(logger) {}

void ClientContext::start_timer(std::chrono::milliseconds time) {
	handler.start_timer(time);
}

void ClientContext::cancel_timer() {
	handler.cancel_timer();
}

void ClientContext::stream_err(const protocol::StreamResult& result) {
	handler.stream_err(result);
}

void ClientContext::state_update(ClientState state) {
	if(state == ClientState::cs_session_closed) {
		handler.stop();
	} else {
		handler.state_update(state);
	}
}

void ClientContext::set_key(std::span<const std::uint8_t> key) {
	connection_->set_key(key);
}

bool ClientContext::packet_logging() {
	return connection_->packet_logging();
}

/*
 * Helper that decides whether to print the IP address or username
 * and IP address in log outputs, based on whether authentication
 * has completed
 */
std::string_view ClientContext::client_identify() const {
	if(client_id) {
		if(client_id_ext_.empty()) {
			client_id_ext_ = std::format(
				"{} ({}, {})", client_id_str_, client_id->username, client_id->id
			);
		}

		return client_id_ext_;
	}

	if(client_id_str_.empty()) [[unlikely]] {
		client_id_str_ = connection_->remote_address();
	}

	return client_id_str_;
}

} // realm, ember