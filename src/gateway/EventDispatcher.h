/*
 * Copyright (c) 2016 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Event.h"
#include "ClientHandler.h"
#include <shared/threading/ServicePool.h>
#include <shared/ClientUUID.h>
#include <concepts>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ember {

class EventDispatcher {
	typedef std::unordered_map<ClientUUID, ClientHandler*> HandlerMap;

	const ServicePool& pool_;
	thread_local static HandlerMap handlers_;

public:
	explicit EventDispatcher(const ServicePool& pool) : pool_(pool) {}

	template<typename T> void exec(const ClientUUID& client, T work) const {
		auto service = pool_.get_service(client.service());

		// bad service index encoded in the UUID
		if(service == nullptr) {
			LOG_ERROR_GLOB << "Invalid service index, " << client.service() << LOG_ASYNC;
			return;
		}

		service->post([client, work] {
			auto handler = handlers_.find(client);

			// client disconnected, nothing to do here
			if(handler == handlers_.end()) {
				LOG_DEBUG_GLOB << "Client disconnected, work discarded" << LOG_ASYNC;
				return;
			}

			work();
		});
	}

	auto post_event(const ClientUUID& client, std::derived_from<Event> auto event) const {
		auto service = pool_.get_service(client.service());

		// bad service index encoded in the UUID
		if(service == nullptr) {
			LOG_ERROR_GLOB << "Invalid service index, " << client.service() << LOG_ASYNC;
			return;
		}

		service->post([=, event = std::move(event)] {
			const auto handler = handlers_.find(client);

			// client disconnected, nothing to do here
			if(handler == handlers_.end()) {
				LOG_DEBUG_GLOB << "Client disconnected, event discarded" << LOG_ASYNC;
				return;
			}

			handler->second->handle_event(&event);
		});
	}

	void post_event(const ClientUUID& client, std::unique_ptr<Event> event) const;
	void broadcast_event(std::vector<ClientUUID> clients, std::shared_ptr<const Event> event) const;
	void register_handler(ClientHandler* handler);
	void remove_handler(ClientHandler* handler);
};

} // ember
