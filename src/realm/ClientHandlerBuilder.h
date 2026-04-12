/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientHandler.h"

namespace ember::realm {

class ClientHandlerBuilder final {
	const ConfigStore& store_;
	EventDispatcher& dispatcher_;
	RealmQueue& queue_;
	AccountClient& account_rpc_;
	CharacterClient& character_rpc_;
	const RealmService& realm_rpc_;
	log::Logger& logger_;

public:
	ClientHandlerBuilder(const ConfigStore& store, EventDispatcher& dispatcher, RealmQueue& queue,
	                     AccountClient& account_rpc, CharacterClient& character_rpc, 
	                     const RealmService& realm_rpc, log::Logger& logger)
		: store_(store)
		, dispatcher_(dispatcher)
		, queue_(queue)
		, account_rpc_(account_rpc)
		, character_rpc_(character_rpc)
		, realm_rpc_(realm_rpc)
		, logger_(logger) {}

	ClientHandler create(std::size_t index, executor executor) const {
		return ClientHandler(
			index, executor, store_, dispatcher_, queue_, account_rpc_, character_rpc_, realm_rpc_, logger_
		);
	}
};

} // realm, ember
