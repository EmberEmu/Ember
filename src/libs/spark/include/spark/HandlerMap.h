/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/temp/MessageRoot_generated.h>
#include <functional>
#include <shared_mutex>
#include <unordered_map>

namespace ember { namespace spark {

class HandlerMap {
	typedef std::function<void(messaging::MessageRoot*)> Handler;

	std::unordered_map<messaging::Service, Handler> handlers_;
	Handler def_handler_;
	mutable std::shared_timed_mutex lock_;

public:
	HandlerMap(Handler default_handler);
	void register_handler(Handler handler, messaging::Service service_type);
	void remove_handler(Handler handler, messaging::Service service_type);
	const Handler& handler(messaging::Service service_type) const;
};

}} // spark, ember