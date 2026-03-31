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
#include <boost/asio/post.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <concepts>
#include <memory>
#include <vector>

namespace ember::realm {

struct CacheSlot {
	ClientIdent ident;
	bool used;
};

class EventDispatcher final {
#ifdef EMBER_FAST_DISPATCH_CACHE
	constexpr static auto dispatch_cache = true;
	constexpr static std::size_t cache_size = ClientIdent::max_slot_value;
	constexpr static auto slot_npos = ClientIdent::max_slot_value;
	static inline thread_local std::array<CacheSlot, cache_size> cache_{};
#endif

	using HandlerMap = boost::unordered_flat_map<
		ClientIdent, ClientHandler*, boost::hash<ClientIdent>
	>;

	const thread::ServicePool& pool_;
	log::Logger& logger_;

	static inline thread_local HandlerMap handlers_;

	inline ClientHandler* locate_handler(const ClientIdent& client) const {
#ifdef EMBER_FAST_DISPATCH_CACHE
		const auto slot = client.extract_slot();

		if(slot != slot_npos) {
			if(cache_[slot].ident == client) {
				return client.extract_ptr<ClientHandler>();
			} else {
				return nullptr;
			}
		}
#endif

		if(auto it = handlers_.find(client); it != handlers_.end()) {
			auto& [_, handler] = *it;
			return handler;
		} else {
			return nullptr;
		}
	}

#ifdef EMBER_FAST_DISPATCH_CACHE
	bool try_insert_cache(ClientHandler* handler);
#endif

	void broadcast(const Event& event) const;

public:
	explicit EventDispatcher(const thread::ServicePool& pool, log::Logger& logger)
		: pool_(pool)
		, logger_(logger) {}

	void exec(const ClientIdent& client, auto work) const {
		auto service = pool_.get_if(client.service());

		// bad service index encoded in the UUID
		if(service == nullptr) {
			LOG_ERROR(logger_, "Invalid service index, {}", client.service());
			return;
		}

		boost::asio::post(*service, [&, client, work = std::move(work)] {
			if(!handlers_.contains(client)) {
				LOG_DEBUG(logger_, "Client disconnected, work discarded");
				return;
			}

			work();
		});
	}

	auto post_event(const ClientIdent& client, std::derived_from<Event> auto event) const {
		auto service = pool_.get_if(client.service());

		// bad service index encoded in the UUID
		if(service == nullptr) {
			LOG_ERROR(logger_, "Invalid service index, {}", client.service());
			return;
		}

		boost::asio::post(*service, [&, client, event = std::move(event)] {
			if(auto handler = locate_handler(client)) {
				handler->handle_event(&event);
			} else {
				LOG_DEBUG(logger_, "Client disconnected, event discarded");
			}
		});
	}

	void post_event(const ClientIdent& client, std::unique_ptr<Event> event) const;

	/*
	 * Broadcasts an event to all handlers, across all service threads
	 */
	void broadcast_event(const Event& event) const;

	/*
	 * Broadcasts an event to all handlers registered to the current thread
	 */
	void broadcast_event_thread(const Event& event) const;

	/*
	* Broadcasts an event to all handlers, across all service threads
	*/
	void broadcast_event(std::shared_ptr<const Event> event) const;
	void broadcast_event(std::vector<ClientIdent> clients, std::shared_ptr<const Event> event) const;
	void register_handler(ClientHandler* handler);
	void remove_handler(const ClientHandler* handler);
};

} // realm, ember
