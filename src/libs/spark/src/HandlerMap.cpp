/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/HandlerMap.h>

namespace ember { namespace spark {

HandlerMap::HandlerMap(MsgHandler default_handler)
                       : def_handler_(default_handler) { }

void HandlerMap::register_handler(MsgHandler handler, LinkStateHandler l_handler,
                                  messaging::Service service_type, Mode mode) {
	std::unique_lock<std::shared_timed_mutex> guard(lock_);
	handlers_[service_type] = { mode, handler, l_handler };
}

void HandlerMap::remove_handler(messaging::Service service_type) {
	std::unique_lock<std::shared_timed_mutex> guard(lock_);
	handlers_.erase(service_type);
}

const LinkStateHandler& HandlerMap::link_state_handler(messaging::Service service_type) const {
	std::shared_lock<std::shared_timed_mutex> guard(lock_);
	auto it = handlers_.find(service_type);

	if(it != handlers_.end()) {
		return it->second.link_handler;
	}

	return def_link_handler_;
}

const MsgHandler& HandlerMap::message_handler(messaging::Service service_type) const  {
	std::shared_lock<std::shared_timed_mutex> guard(lock_);
	auto it = handlers_.find(service_type);

	if(it != handlers_.end()) {
		return it->second.msg_handler;
	}

	return def_handler_;
}

std::vector<messaging::Service> HandlerMap::services(Mode mode) const {
	std::shared_lock<std::shared_timed_mutex> guard(lock_);
	std::vector<messaging::Service> services;

	for(auto& handler : handlers_) {
		if(handler.second.mode == mode || handler.second.mode == Mode::BOTH) {
			services.emplace_back(handler.first);
		}
	}

	return services;
}

}} // spark, ember