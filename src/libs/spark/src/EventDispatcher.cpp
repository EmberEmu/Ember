/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/EventDispatcher.h>

namespace ember::spark::inline v1 {

void EventDispatcher::register_handler(EventHandler* handler, messaging::Service service, Mode mode) {
	std::unique_lock<std::shared_mutex> guard(lock_);
	handlers_[service] = { mode, handler };
}

/* Remove by pointer rather than service to reduce the odds of making the
   mistake of removing a handler that doesn't belong to the caller */
void EventDispatcher::remove_handler(const EventHandler* handler) {
	std::unique_lock<std::shared_mutex> guard(lock_);
	
	for(auto& entry: handlers_) {
		if(entry.second.handler == handler) {
			handlers_.erase(entry.first);
			break;
		}
	}
}

void EventDispatcher::notify_link_up(messaging::Service service, const Link& link) const {
	std::lock_guard<std::shared_mutex> guard(lock_);

	auto it = handlers_.find(service);

	if(it != handlers_.end()) {
		it->second.handler->on_link_up(link);
	}
}

void EventDispatcher::notify_link_down(messaging::Service service, const Link& link) const {
	std::lock_guard<std::shared_mutex> guard(lock_);

	auto it = handlers_.find(service);

	if(it != handlers_.end()) {
		it->second.handler->on_link_down(link);
	}
}

void EventDispatcher::dispatch_message(messaging::Service service, const Link& link,
                                       const Message& message) const {
	std::unique_lock<std::shared_mutex> guard(lock_);
	auto it = handlers_.find(service);

	if(it != handlers_.end()) {
		it->second.handler->on_message(link, message);
	}
} 

std::vector<messaging::Service> EventDispatcher::services(Mode mode) const {
	std::shared_lock<std::shared_mutex> guard(lock_);
	std::vector<messaging::Service> services;

	for(auto& handler : handlers_) {
		if(handler.second.mode == mode || handler.second.mode == Mode::BOTH) {
			services.emplace_back(handler.first);
		}
	}

	return services;
}

} // spark, ember