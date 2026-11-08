/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "EventDispatcher.h"
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>

namespace ember::realm {

void EventDispatcher::deliver(const is_event auto& event) const {
#ifndef DISABLE_FAST_DISPATCH_TABLE
	for(auto& entry : cache_) {
		if(!entry.is_zero()) {
			entry.ptr<Client>()->handle_event(event);
		}
	}
#endif
	for(auto& handler : handlers_ | std::views::values) {
		handler->handle_event(event);
	}
}

inline Client* EventDispatcher::locate_handler(const ClientIdent& client) const {
#ifndef DISABLE_FAST_DISPATCH_TABLE
	if(auto ptr = client.ptr<Client>()) {
		return cache_[client.slot()] == client? ptr : nullptr;
	}
#endif

	if(auto it = handlers_.find(client); it != handlers_.end()) {
		return it->second;
	} else {
		return nullptr;
	}
}

void EventDispatcher::execute(const ClientIdent& client, auto&& work) const {
	auto service = pool_.get_if(client.service());

	// bad service index encoded in the UUID
	if(service == nullptr) {
		LOG_ERROR(logger_, "Invalid service index, {}", client.service());
		return;
	}

	boost::asio::post(*service, [&, client, work = std::move(work)] {
		if(auto handler = locate_handler(client)) {
			if constexpr(std::invocable<decltype(work), const Client&>) {
				work(*handler);
			} else {
				work();
			}
		} else {
			LOG_DEBUG(logger_, "Client disconnected, work discarded");
		}
	});
}

void EventDispatcher::dispatch(const ClientIdent& client, is_event auto event) const {
	// bypass Asio if the client is on the same thread
	if(auto handler = locate_handler(client)) {
		handler->handle_event(event);
		return;
	}

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

void EventDispatcher::post(const ClientIdent& client, is_event auto event) const {
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

void EventDispatcher::broadcast(const is_event auto& event) const {
	for(auto i = 0u; i < pool_.size(); ++i) { // todo size_t literal when min. compiler bump
		broadcast_worker(i, event);
	}
}

void EventDispatcher::broadcast_self(const is_event auto& event) const {
	deliver(event);
}

void EventDispatcher::broadcast_worker(std::size_t index, is_event auto event) const {
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

} // realm, ember