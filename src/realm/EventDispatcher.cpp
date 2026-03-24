/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "EventDispatcher.h"
#include <logger/Logger.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <type_traits>

namespace ember::realm {

void EventDispatcher::post_event(const ClientRef& client, std::unique_ptr<Event> event) const {
	auto service = pool_.get_if(client.service());

	// bad service index encoded in the UUID
	if(service == nullptr) {
		LOG_ERROR_ASYNC(logger_, "Invalid service index, {}", client.service());
		return;
	}

	boost::asio::post(*service, [&, client, event = std::move(event)] {
		if(auto handler = handlers_.find(client); handler != handlers_.end()) {
			handler->second->handle_event(event.get());
		} else {
			LOG_DEBUG_ASYNC(logger_, "Client disconnected, event discarded");
		}
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
void EventDispatcher::broadcast_event(std::vector<ClientRef> clients, std::shared_ptr<const Event> event) const {
	std::ranges::sort(clients, [](auto& lhs, auto& rhs) {
		return lhs.service() < rhs.service();
	});

	const auto clients_ptr = std::make_shared<decltype(clients)>();
	clients_ptr->swap(clients);

	for(std::size_t i = 0, j = pool_.size(); i < j; ++i) {
		const auto service_id = gsl::narrow<std::uint8_t>(i);

		const auto found = std::ranges::binary_search(
			*clients_ptr, service_id, std::ranges::less{}, &ClientRef::service
		);

		if(!found) {
			continue;
		}

		const auto range = std::ranges::equal_range(
			*clients_ptr, service_id, std::ranges::greater{}, &ClientRef::service
		);

		auto& service = pool_.get(i);

		boost::asio::post(service, [&, clients_ptr, range, event] {
			auto [beg, end] = range;

			while(beg != end) {
				if(auto handler = handlers_.find(*beg++); handler != handlers_.end()) {
					handler->second->handle_event(event.get());
				} else {
					LOG_DEBUG_ASYNC(logger_, "Client disconnected, event discarded");
				}
			}
		});
	}
}

void EventDispatcher::broadcast_event(const Event& event) const {
	for(auto& ioc : pool_) {
		boost::asio::dispatch(*ioc, [event]() {
			for(auto& handler : handlers_ | std::views::values) {
				handler->handle_event(&event);
			}
		});
	}
}

void EventDispatcher::broadcast_event(std::shared_ptr<const Event> event) const {
	for(auto& ioc : pool_) {
		boost::asio::dispatch(*ioc, [event]() {
			for(auto& handler : handlers_ | std::views::values) {
				handler->handle_event(event.get());
			}
		});
	}
}

void EventDispatcher::register_handler(ClientHandler* handler) {
	auto& service = pool_.get(handler->uuid().service());

	boost::asio::dispatch(service, [=] {
		handlers_.insert_or_assign(handler->uuid(), handler);
	});
}

void EventDispatcher::remove_handler(const ClientHandler* handler) {
	auto& service = pool_.get(handler->uuid().service());

	boost::asio::dispatch(service, [=] {
		handlers_.erase(handler->uuid());
	});
}

} // realm, ember
