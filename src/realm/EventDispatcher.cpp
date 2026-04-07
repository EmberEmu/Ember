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

void EventDispatcher::post_event(const ClientIdent& client, std::unique_ptr<Event> event) const {
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

/*
 * This function is intended only for broadcasts of a single event to a
 * large number of clients. The goal here is to minimise the number of
 * posts required to dispatch the events to all specified clients, given that
 * it's the most expensive aspect of the event handling process.
 *
 * Callers should move the client UUID vector into this function.
 */
void EventDispatcher::broadcast_event(std::vector<ClientIdent> clients, std::shared_ptr<const Event> event) const {
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
			*clients_ptr, service_id, std::ranges::greater{}, &ClientIdent::service
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

void EventDispatcher::broadcast_event(std::shared_ptr<const Event> event) const {
	for(auto& ioc : pool_.services()) {
		boost::asio::dispatch(ioc, [&, event]() {
			dispatch_event(*event.get());
		});
	}
}

#ifdef EMBER_FAST_DISPATCH_CACHE
#pragma warning(push)
#pragma warning(disable : 28020) // ignore false positive
bool EventDispatcher::try_insert(ClientHandler* handler) {
	assert(handler);

	std::size_t index = rng::xorshift::next() & 0xfff;

	// modulo avoidance, mask needs to be a power of two
	// bump down if we were unlucky enough to hit the max
	if(index == 0xfff) {
		index = 0;
	}

	const std::size_t start = index;

	do {
		if(cache_[index].is_zero()) {
			handler->uuid().encode(handler, index);
			cache_[index] = handler->uuid();
			return true;
		} else if(++index == slot_npos) {
			index = 0;
		}
	} while(index != start);

	handler->uuid().encode(0, slot_npos); // encode sentinel
	return false;
}
#pragma warning(pop)
#endif

void EventDispatcher::register_handler(ClientHandler* handler) {
	assert(handler);

#ifdef EMBER_FAST_DISPATCH_CACHE
	if(!try_insert(handler)) {
#endif
		handlers_.insert_or_assign(handler->uuid(), handler);
#ifdef EMBER_FAST_DISPATCH_CACHE
	}
#endif
}

void EventDispatcher::remove_handler(const ClientHandler* handler) {
#ifdef EMBER_FAST_DISPATCH_CACHE
		if(auto slot = handler->uuid().extract_slot(); slot != slot_npos) {
			cache_[slot].set_zero();
		}  else {
			handlers_.erase(handler->uuid());
		}
#else
	handlers_.erase(handler->uuid());
#endif
}

} // realm, ember
