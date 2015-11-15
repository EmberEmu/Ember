/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/HandlerMap.h>

namespace ember { namespace spark {

HandlerMap::HandlerMap(Handler default_handler) : def_handler_(default_handler) { }

void HandlerMap::register_handler(Handler handler, messaging::Service service_type) {
	std::unique_lock<std::shared_timed_mutex> guard(lock_);
	handlers_[service_type] = handler;
}

void HandlerMap::remove_handler(Handler handler, messaging::Service service_type) {
	std::unique_lock<std::shared_timed_mutex> guard(lock_);
	handlers_.erase(service_type);
}

auto HandlerMap::handler(messaging::Service service_type) const -> const Handler& {
	std::shared_lock<std::shared_timed_mutex> guard(lock_);
	auto& handler = handlers_.at(service_type);
	return handler? handler : def_handler_;
}

}} // spark, ember