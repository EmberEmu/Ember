/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Common.h>
#include <spark/EventHandler.h>
#include <spark/Link.h>
#include <spark/temp/MessageRoot_generated.h>
#include <spark/temp/ServiceTypes_generated.h>
#include <forward_list>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace ember { namespace spark {

class EventDispatcher {
public:
	enum class Mode { CLIENT, SERVER, BOTH };

private:
	struct Handler {
		Mode mode;
		EventHandler* handler;
	};

	std::unordered_map<messaging::Service, Handler> handlers_;
	mutable std::shared_timed_mutex lock_;

public:
	std::vector<messaging::Service> services(Mode mode) const;
	void register_handler(EventHandler* handler, messaging::Service service, Mode mode);
	void remove_handler(EventHandler* handler);
	void dispatch_link_event(messaging::Service service, const Link& link, LinkState state) const;
	void dispatch_message(messaging::Service service, const Link& link, const messaging::MessageRoot* message) const;
};

}} // spark, ember