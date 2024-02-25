/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Services_generated.h"
#include <spark/Common.h>
#include <spark/EventHandler.h>
#include <spark/Link.h>
#include <forward_list>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace ember::spark::inline v1 {

class EventDispatcher {
public:
	enum class Mode { CLIENT, SERVER, BOTH };

private:
	struct Handler {
		Mode mode;
		EventHandler* handler;
	};

	std::unordered_map<messaging::Service, Handler> handlers_;
	mutable std::shared_mutex lock_;

public:
	std::vector<messaging::Service> services(Mode mode) const;
	void register_handler(EventHandler* handler, messaging::Service service, Mode mode);
	void remove_handler(const EventHandler* handler);
	void notify_link_up(messaging::Service service, const Link& link) const;
	void notify_link_down(messaging::Service service, const Link& link) const;
	void dispatch_message(messaging::Service service, const Link& link, const Message& message) const;
};

} // spark, ember