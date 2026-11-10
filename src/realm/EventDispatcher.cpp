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
#include <format>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <cassert>

namespace ember::realm {

EventDispatcher::EventDispatcher(const thread::ServicePool& pool, log::Logger& logger)
	: pool_(pool)
	, logger_(logger) {
	if(pool_.size() > ClientIdent::service_count) {
		throw std::invalid_argument(
			std::format(
				"Service pool too large for client encoding; assign more bits or "
				"restrict concurrency; limit is {}", ClientIdent::service_count
			)
		);
	}
}

std::size_t EventDispatcher::random_slot() const {
	static_assert((ClientIdent::max_slot & (ClientIdent::max_slot + 1)) == 0, "bad mask value");
	return rng::xorshift::next() & ClientIdent::max_slot;
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

	for(std::size_t service_id = 0, j = pool_.size(); service_id < j; ++service_id) {
		const auto range = std::ranges::equal_range(
			*clients_ptr, service_id, std::ranges::less{}, &ClientIdent::service
		);

		if(range.empty()) {
			continue;
		}

		auto& service = pool_.get(service_id);

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
bool EventDispatcher::try_insert(Client* handler, ClientIdent& ident) {
	assert(handler);

	if(next_slot_ == slot_full) {
		return false;
	}

	auto index = next_slot_;

	do {
		if(!cache_[index]) {
			ident.encode(handler, index);
			cache_[index] = ident;
			next_slot_ = random_slot(); // jump off point for next insertion
			return true;
		}

		index = (index + 1) & slot_mask;
	} while(index != next_slot_);

	next_slot_ = slot_full;     // mark table full
	ident.encode(nullptr, 0);   // encode sentinel
	return false;
}
#endif

ClientIdent EventDispatcher::register_client(Client* client) {
	assert(client);
	assert(pool_.running_in_pool());

	ClientIdent ident(
		pool_.current_index()
	);

#ifndef DISABLE_FAST_DISPATCH_TABLE
	if(!try_insert(client, ident))
#endif
		handlers_.insert_or_assign(ident, client);

	return ident;
}

void EventDispatcher::remove_client(const ClientIdent& ident) {
#ifndef DISABLE_FAST_DISPATCH_TABLE
	if(ident.ptr<Client>()) {
		const auto slot = ident.slot();

		if(cache_[slot] == ident) {
			cache_[slot].clear();
			next_slot_ = slot;
		}

		return;
	}
#endif

	handlers_.erase(ident);
}

} // realm, ember
