/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Common.h>
#include <spark/Link.h>
#include <spark/temp/MessageRoot_generated.h>
#include <spark/temp/ServiceTypes_generated.h>
#include <functional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace ember { namespace spark {

// todo, rename?
class HandlerMap {
public:
	enum class Mode { CLIENT, SERVER, BOTH };

private:
	struct Handlers {
		Mode mode;
		MsgHandler msg_handler;
		LinkStateHandler link_handler;
	};

	std::unordered_map<messaging::Service, Handlers> handlers_;
	MsgHandler def_handler_;
	LinkStateHandler def_link_handler_;
	mutable std::shared_timed_mutex lock_;


public:
	HandlerMap(MsgHandler default_handler);
	void register_handler(MsgHandler handler, LinkStateHandler l_handler,
	                      messaging::Service service_type, Mode mode);
	void remove_handler(messaging::Service service_type);
	const MsgHandler& message_handler(messaging::Service service_type) const;
	const LinkStateHandler& link_state_handler(messaging::Service service_type) const;
	std::vector<messaging::Service> services(Mode mode) const;
};

}} // spark, ember