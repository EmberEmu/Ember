/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Forwards.h"
#include "unique_client_ptr.h"
#include <memory>

namespace ember::realm {

class ClientBuilder {
	constexpr static auto allocator_tag = "client_allocator";

	ClientAllocator allocator_;
	const ConfigStore& store_;
	EventDispatcher& dispatcher_;
	RealmQueue& queue_;
	AccountClient& account_rpc_;
	CharacterClient& character_rpc_;
	log::Logger& logger_;

	unique_client_ptr make_unique_client(tcp_socket socket, ClientIdent ident) {
		return unique_client_ptr(allocator_.allocate(
			std::move(socket), std::move(ident), store_, dispatcher_, queue_,
			account_rpc_, character_rpc_, logger_
		), ClientDeleter(&allocator_));
	}

public:
	ClientBuilder(const ConfigStore& store,
	              EventDispatcher& dispatcher,
	              RealmQueue& queue,
	              AccountClient& account_rpc,
	              CharacterClient& character_rpc,
	              log::Logger& logger)
		: allocator_(allocator_tag)
		, store_(store)
		, dispatcher_(dispatcher)
		, queue_(queue)
		, account_rpc_(account_rpc)
		, character_rpc_(character_rpc)
		, logger_(logger) {}

	unique_client_ptr create(tcp_socket socket, ClientIdent ident) {
		return make_unique_client(std::move(socket), std::move(ident));
	}
};

} // realm, ember
