/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "EventDispatcher.h"
#include <logger/Logging.h>
#include <boost/asio/post.hpp>
#include <type_traits>

namespace ember {

void EventDispatcher::post_event(const ClientUUID& client, std::unique_ptr<Event> event) const {
	auto service = pool_.get_service(client.service());

	// bad service index encoded in the UUID
	if(service == nullptr) {
		LOG_ERROR_GLOB << "Invalid service index, " << client.service() << LOG_ASYNC;
		return;
	}

	boost::asio::post(*service, [client, event = std::move(event)] {
		const auto handler = handlers_.find(client);

		if(handler == handlers_.end()) {
			LOG_DEBUG_GLOB << "Client disconnected, event discarded" << LOG_ASYNC;
			return;
		}

		handler->second->handle_event(event.get());
	});
}

/*
 * This function is intended only for broadcasts of a single event to a
 * large number of clients. The goal here is to minimise the number of
 * posts required to dispatch the events to all specified clients, given that
 * it's the most expensive aspect of the event handling process.
 *
 * Callers should move the client UUID vector into this function.
 */
void EventDispatcher::broadcast_event(std::vector<ClientUUID> clients,
                                      std::shared_ptr<const Event> event) const {
	std::sort(clients.begin(), clients.end(), [](auto& lhs, auto& rhs) {
		return lhs.service() < rhs.service();
	});

	const auto clients_ptr = std::make_shared<decltype(clients)>();
	clients_ptr->swap(clients);

	const auto size = pool_.size();

	for(std::remove_const<decltype(size)>::type i = 0; i < size; ++i) {
		const auto uuid = ClientUUID::generate(i);

		const auto found = std::binary_search(clients_ptr->begin(), clients_ptr->end(), uuid,
			[](auto& lhs, auto& rhs) {
				return lhs.service() < rhs.service();
			}
		);

		if(!found) {
			continue;
		}

		const auto range = std::equal_range(clients_ptr->begin(), clients_ptr->end(), uuid,
			[](auto& lhs, auto& rhs) {
				return lhs.service() > rhs.service();
			}
		);

		auto service = pool_.get_service(i);

		service->post([clients_ptr, range, event = std::move(event)] {
			auto it = range.first;

			while(it != range.second) {
				const auto handler = handlers_.find(*it++);

				if(handler == handlers_.end()) {
					LOG_DEBUG_GLOB << "Client disconnected, event discarded" << LOG_ASYNC;
					continue;
				}

				handler->second->handle_event(event.get());
			}
		});
	}
}

void EventDispatcher::register_handler(ClientHandler* handler) {
	auto service = pool_.get_service(handler->uuid().service());

	service->dispatch([=] {
		handlers_[handler->uuid()] = handler;
	});
}

void EventDispatcher::remove_handler(const ClientHandler* handler) {
	auto service = pool_.get_service(handler->uuid().service());

	service->dispatch([=] {
		handlers_.erase(handler->uuid());
	});
}

} // ember
