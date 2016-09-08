/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "EventDispatcher.h"


namespace ember {

EventDispatcher::HandlerMap EventDispatcher::handlers_;

void EventDispatcher::post_event(const client_uuid::uuid& client, std::shared_ptr<Event> event) {
	auto service = pool_.get_service(client.service);

	if(service == nullptr) {
		// log
		return;
	}

	service->dispatch([=] {
		auto handler = handlers_.find(client);

		// client disconnected, nothing to do here
		if(handler == handlers_.end()) {
			return;
		}
		
		handler->second->handle_event(event);
	});
}

void EventDispatcher::register_handler(ClientHandler* handler) {
	handlers_[handler->uuid()] = handler;
}

void EventDispatcher::remove_handler(ClientHandler* handler) {
    handlers_.erase(handler->uuid());
}

} // ember