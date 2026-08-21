/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "EventDispatcher.h"
#include <logger/Logger.h>
#include <shared/utility/xoroshiro128plus.h>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <ranges>
#include <type_traits>
#include <cassert>

namespace ember::realm {

void EventDispatcher::post(const ClientIdent& client, std::unique_ptr<Event> event) const {
	auto service = pool_.get_if(client.service());

	// bad service index encoded in the UUID
	if(service == nullptr) {
		LOG_ERROR(logger_, "Invalid service index, {}", client.service());
		return;
	}

	boost::asio::post(*service, [&, client, event = std::move(event)] {
		if(auto handler = locate_handler(client)) {
			handler->handle_event(*event);
		} else {
			LOG_DEBUG(logger_, "Client disconnected, event discarded");
		}
	});
}

void EventDispatcher::deliver(const Event& event) const {
#ifndef DISABLE_FAST_DISPATCH_TABLE
	for(auto& entry : cache_) {
		if(!entry.is_zero()) {
			entry.extract_ptr<ClientType>()->handle_event(event);
		}
	}
#endif
	for(auto& handler : handlers_ | std::views::values) {
		handler->handle_event(event);
	}
}

Client* EventDispatcher::locate_handler(const ClientIdent& client) const {
#ifndef DISABLE_FAST_DISPATCH_TABLE
	const auto slot = client.extract_slot();

	if(slot != slot_npos) {
		if(cache_[slot] == client) {
			return client.extract_ptr<ClientType>();
		} else {
			return nullptr;
		}
	}
#endif

	if(auto it = handlers_.find(client); it != handlers_.end()) {
		return it->second;
	} else {
		return nullptr;
	}
}

void EventDispatcher::dispatch(const ClientIdent& client, Event event) const {
	auto service = pool_.get_if(client.service());

	// bad service index encoded in the UUID
	if(service == nullptr) {
		LOG_ERROR(logger_, "Invalid service index, {}", client.service());
		return;
	}

	boost::asio::dispatch(*service, [&, client, event = std::move(event)] {
		if(auto handler = locate_handler(client)) {
			handler->handle_event(event);
		} else {
			LOG_DEBUG(logger_, "Client disconnected, event discarded");
		}
	});
}

void EventDispatcher::post(const ClientIdent& client, Event event) const {
	auto service = pool_.get_if(client.service());

	// bad service index encoded in the UUID
	if(service == nullptr) {
		LOG_ERROR(logger_, "Invalid service index, {}", client.service());
		return;
	}

	boost::asio::post(*service, [&, client, event = std::move(event)] {
		if(auto handler = locate_handler(client)) {
			handler->handle_event(event);
		} else {
			LOG_DEBUG(logger_, "Client disconnected, event discarded");
		}
	});
}

void EventDispatcher::broadcast(const Event& event) const {
	for(auto i = 0u; i < pool_.size(); ++i) { // todo size_t literal when min. compiler bump
		broadcast_worker(i, event);
	}
}

void EventDispatcher::broadcast_self(Event event) const {
	deliver(event);
}

void EventDispatcher::broadcast_worker(std::size_t index, Event event) const {
	auto service = pool_.get_if(index);

	// bad service index encoded in the UUID
	if(service == nullptr) {
		LOG_ERROR(logger_, "Invalid service index, {}", index);
		return;
	}

	boost::asio::post(*service, [&, event = std::move(event)]() {
		deliver(event);
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
void EventDispatcher::broadcast(std::vector<ClientIdent> clients, std::shared_ptr<const Event> event) const {
	std::ranges::sort(clients, [](auto& lhs, auto& rhs) {
		return lhs.service() < rhs.service();
	});

	const auto clients_ptr = std::make_shared<decltype(clients)>();
	clients_ptr->swap(clients);

	for(std::size_t i = 0, j = pool_.size(); i < j; ++i) {
		const auto service_id = gsl::narrow<std::uint8_t>(i);

		const auto found = std::ranges::binary_search(
			*clients_ptr, service_id, std::ranges::less{}, &ClientIdent::service
		);

		if(!found) {
			continue;
		}

		const auto range = std::ranges::equal_range(
			*clients_ptr, service_id, std::ranges::less{}, &ClientIdent::service
		);

		auto& service = pool_.get(i);

		boost::asio::post(service, [&, clients_ptr, range, event] {
			auto [beg, end] = range;

			while(beg != end) {
				if(auto handler = locate_handler(*beg++)) {
					handler->handle_event(*event);
				} else {
					LOG_DEBUG(logger_, "Client disconnected, event discarded");
				}
			}
		});
	}
}

void EventDispatcher::broadcast(std::shared_ptr<const Event> event) const {
	for(auto& ioc : pool_.services()) {
		boost::asio::dispatch(ioc, [&, event]() {
			deliver(*event.get());
		});
	}
}

#ifndef DISABLE_FAST_DISPATCH_TABLE
#pragma warning(push)
#pragma warning(disable : 28020) // ignore false positive
bool EventDispatcher::try_insert(ClientType* handler, ClientIdent& ident) {
	assert(handler);

	std::size_t index = rng::xorshift::next() & 0xfff;

	// modulo avoidance, mask needs to be a power of two
	// bump down if we were unlucky enough to hit the max
	if(index == 0xfff) {
		index = 0;
	}

	const auto start = index;

	do {
		if(cache_[index].is_zero()) {
			ident.encode(handler, index);
			cache_[index] = ident;
			return true;
		} else if(++index == slot_npos) {
			index = 0;
		}
	} while(index != start);

	ident.encode(0, slot_npos); // encode sentinel
	return false;
}
#pragma warning(pop)
#endif

ClientIdent EventDispatcher::register_client(ClientType* client, std::size_t service_index) {
	assert(client);
	ClientIdent ident(service_index);

#ifndef DISABLE_FAST_DISPATCH_TABLE
	if(!try_insert(client, ident))
#endif
		handlers_.insert_or_assign(ident, client);

	return ident;
}

void EventDispatcher::remove_client(const ClientType* client) {
	assert(client);

#ifndef DISABLE_FAST_DISPATCH_TABLE
	if(auto slot = client->uuid().extract_slot(); slot != slot_npos) {
		cache_[slot].set_zero();
	}  else {
		handlers_.erase(client->uuid());
	}
#else
	handlers_.erase(client->uuid());
#endif
}

} // realm, ember
