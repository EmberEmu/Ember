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

std::size_t EventDispatcher::random_slot() const {
	constexpr auto max_slot = ClientIdent::max_slot_value;
	static_assert((max_slot & (max_slot + 1)) == 0, "bad mask value");

	auto index = rng::xorshift::next() & max_slot;

	// modulo avoidance, mask needs to be a power of two
	// bump down if we were unlucky enough to hit the max
	if(index == max_slot) {
		index = 0;
	}

	return index;
}

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

	const auto clients_ptr = std::make_shared<decltype(clients)>(std::move(clients));

	for(std::size_t i = 0, j = pool_.size(); i < j; ++i) {
		const auto service_id = gsl::narrow<std::uint8_t>(i);

		const auto range = std::ranges::equal_range(
			*clients_ptr, service_id, std::ranges::less{}, &ClientIdent::service
		);

		if(range.empty()) {
			continue;
		}

		auto& service = pool_.get(i);

		boost::asio::post(service, [this, clients_ptr, range, event] {
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
bool EventDispatcher::try_insert(ClientType* handler, ClientIdent& ident) {
	assert(handler);

	auto index = next_slot_;

	if(index == slot_full) {
		return false;
	}

	const auto start = index;

	do {
		if(cache_[index].is_zero()) {
			ident.encode(handler, index);
			cache_[index] = ident;
			next_slot_ = random_slot(); // jump off point for next insertion
			return true;
		} else if(++index == slot_npos) {
			index = 0;
		}
	} while(index != start);

	next_slot_ = slot_full;     // mark table full
	ident.encode(0, slot_npos); // encode sentinel
	return false;
}
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
	if(auto slot = client->ident().extract_slot(); slot != slot_npos) {
		cache_[slot].set_zero();
		next_slot_ = slot;
	} else {
		handlers_.erase(client->ident());
	}
#else
	handlers_.erase(client->ident());
#endif
}

} // realm, ember
