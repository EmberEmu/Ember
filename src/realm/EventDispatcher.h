/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Event.h"
#include "Client.h"
#include <logger/Logger.h>
#include <thread/ServicePool.h>
#include <shared/ClientIdent.h>
#include <boost/functional/hash.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <concepts>
#include <memory>
#include <vector>
#include <cstddef>

namespace ember::realm {

template<typename T>
concept is_event = std::derived_from<T, Event>;

class EventDispatcher final {
	using ClientType = Client;

	using HandlerMap = boost::unordered_flat_map<
		ClientIdent, Client*, boost::hash<ClientIdent>
	>;

#ifdef EMBER_FAST_DISPATCH_CACHE
	constexpr static std::size_t cache_size = ClientIdent::max_slot_value;
	constexpr static auto slot_npos = ClientIdent::max_slot_value;
	static inline thread_local std::array<ClientIdent, cache_size> cache_{};
#endif

	static inline thread_local HandlerMap handlers_;

	const thread::ServicePool& pool_;
	log::Logger& logger_;

	inline Client* locate_handler(const ClientIdent& client) const;
	inline void deliver(const is_event auto& event) const;

#ifdef EMBER_FAST_DISPATCH_CACHE
	bool try_insert(ClientType* handler, ClientIdent& ident);
#endif

public:
	explicit EventDispatcher(const thread::ServicePool& pool, log::Logger& logger)
		: pool_(pool)
		, logger_(logger) {}

	// execute a task, only if the specified client still exists
	void exec(const ClientIdent& client, auto work) const;

	// post an event to a specific client, if it's still connected
	auto post(const ClientIdent& client, is_event auto event) const;

	// dispatch an event to a specific client, if it's still connected
	// if caller resides on same worker as client, event will be handled synchronously
	auto dispatch(const ClientIdent& client, is_event auto event) const;

	// post an event to a specific client, if it's still connected
	void post(const ClientIdent& client, std::unique_ptr<Event> event) const;

	// broadcasts an event to all handlers, across all service threads/workers
	void broadcast(const is_event auto& event) const;

	// broadcasts an event to all handlers registered to the specified thread
	void broadcast_worker(std::size_t index, is_event auto event) const;

	// broadcasts an event to all handlers registered with the worker residing on the current thread
	// has no effect if not called from within a worker
	void broadcast_self(is_event auto event) const;

	// broadcasts an event to all handlers, across all service threads
	void broadcast(std::shared_ptr<const Event> event) const;

	// broadcasts an event to a vector of clients - the vector is sorted to minimise the number of messages
	void broadcast(std::vector<ClientIdent> clients, std::shared_ptr<const Event> event) const;

	ClientIdent register_client(Client* client, std::size_t service_index);
	void remove_client(const Client* client);
};

} // realm, ember

#include "EventDispatcher.inl"