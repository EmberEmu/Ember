/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Event.h"
#include "ClientHandler.h"
#include <logger/Logger.h>
#include <thread/ServicePool.h>
#include <shared/ClientIdent.h>
#include <boost/functional/hash.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <concepts>
#include <memory>
#include <vector>

namespace ember::realm {

class EventDispatcher final {
	using HandlerMap = boost::unordered_flat_map<
		ClientIdent, ClientHandler*, boost::hash<ClientIdent>
	>;

#ifdef EMBER_FAST_DISPATCH_CACHE
	constexpr static std::size_t cache_size = ClientIdent::max_slot_value;
	constexpr static auto slot_npos = ClientIdent::max_slot_value;
	static inline thread_local std::array<ClientIdent, cache_size> cache_{};
#endif

	static inline thread_local HandlerMap handlers_;

	const thread::ServicePool& pool_;
	log::Logger& logger_;

	inline ClientHandler* locate_handler(const ClientIdent& client) const;
	inline void dispatch_event(const std::derived_from<Event> auto& event) const;

#ifdef EMBER_FAST_DISPATCH_CACHE
	bool try_insert(ClientHandler* handler);
#endif

public:
	explicit EventDispatcher(const thread::ServicePool& pool, log::Logger& logger)
		: pool_(pool)
		, logger_(logger) {}

	// execute a task, only if the specified client still exists
	void exec(const ClientIdent& client, auto work) const;

	// post an event to a specific client, if it's still connected
	auto post_event(const ClientIdent& client, std::derived_from<Event> auto event) const;

	// post an event to a specific client, if it's still connected
	void post_event(const ClientIdent& client, std::unique_ptr<Event> event) const;

	// broadcasts an event to all handlers, across all service threads/workers
	void broadcast_event(const std::derived_from<Event> auto& event) const;

	// broadcasts an event to all handlers registered to the specified thread
	void broadcast_event_worker(std::size_t index, std::derived_from<Event> auto event) const;

	// broadcasts an event to all handlers registered with the worker residing on the current thread
	// this should only be called from within a worker
	void broadcast_event_self(std::derived_from<Event> auto event) const;

	// broadcasts an event to all handlers, across all service threads
	void broadcast_event(std::shared_ptr<const Event> event) const;

	// broadcasts an event to a vector of clients - the vector is sorted to minimise the number of messages
	void broadcast_event(std::vector<ClientIdent> clients, std::shared_ptr<const Event> event) const;

	void register_handler(ClientHandler* handler);
	void remove_handler(const ClientHandler* handler);
};

} // realm, ember

#include "EventDispatcher.inl"