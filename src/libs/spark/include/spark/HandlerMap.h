/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Link.h>
#include <spark/temp/MessageRoot_generated.h>
#include <functional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace ember { namespace spark {

typedef std::function<void(const Link& link, const messaging::MessageRoot*)> MsgHandler;
typedef std::function<void(const Link& link, LinkState state)> LinkStateHandler;

// todo, rename?
class HandlerMap {
	typedef std::pair<MsgHandler, LinkStateHandler> HandlerPair;

	std::vector<messaging::Service> outbound_services_;
	std::unordered_map<messaging::Service, HandlerPair> handlers_;
	MsgHandler def_handler_;
	LinkStateHandler def_link_handler_;
	mutable std::shared_timed_mutex lock_;

public:
	HandlerMap(MsgHandler default_handler);
	void register_handler(MsgHandler handler, LinkStateHandler l_handler, messaging::Service service_type);
	void remove_handler(messaging::Service service_type);
	const MsgHandler& message_handler(messaging::Service service_type) const;
	const LinkStateHandler& link_state_handler(messaging::Service service_type) const;
	std::vector<messaging::Service> inbound_services() const;
	std::vector<messaging::Service> outbound_services() const;
	void register_outbound(messaging::Service services);

};

}} // spark, ember