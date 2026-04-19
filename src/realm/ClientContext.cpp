/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientContext.h"
#include "EventDispatcher.h"
#include "ClientHandler.h"
#include <format>

namespace ember::realm {

ClientContext::ClientContext(executor& executor, const ConfigStore& cfg_store, EventDispatcher& dispatcher,
                             RealmQueue& queue, AccountClient& account_rpc, CharacterClient& character_rpc,
                             const RealmService& realm_rpc, log::Logger& logger)
	: timer_(executor)
	, cfg_store(cfg_store)
	, dispatcher(dispatcher)
	, queue(queue)
	, account_rpc(account_rpc)
	, character_rpc(character_rpc)
	, realm_rpc(realm_rpc)
	, logger(logger) {}

void ClientContext::start_timer(const std::chrono::milliseconds& time) {
	timer_.expires_after(time);

	timer_.async_wait([dispatcher = dispatcher, uuid = handler().uuid()](const boost::system::error_code& ec) {
		if(!ec) {
			Event event { EventType::timer_expired };
			dispatcher.post(uuid, event);
		}
	});
}

void ClientContext::cancel_timer() {
	timer_.cancel_one();
}

void ClientContext::stream_err(const protocol::StreamResult& result) {
	handler().stream_err(result);
}

void ClientContext::state_update(ClientState state) {
	if(state == ClientState::cs_session_closed) {
		handler().close_session();
	} else {
		handler().state_update(state);
	}
}

void ClientContext::set_key(std::span<const std::uint8_t, ClientConnection::key_size> key) {
	connection_->set_key(key);
}

bool ClientContext::packet_logging() const {
	return connection_->packet_logging();
}

/*
 * Helper that decides whether to print the IP address or username
 * and IP address in log outputs, based on whether authentication
 * has completed
 */
std::string_view ClientContext::whoami() const {
	if(account) {
		if(client_id_ext_.empty()) {
			client_id_ext_ = std::format(
				"{} ({}, {})", client_id_str_, account->username, account->id
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