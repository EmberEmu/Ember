/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Event.h"
#include "ClientHandler.h"
#include "ServicePool.h"
#include <shared/ClientUUID.h>
#include <queue>
#include <memory>
#include <unordered_map>
#include <boost/asio/io_service.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/functional/hash.hpp>

namespace ember {

class EventDispatcher {
	typedef std::unordered_map<ClientUUID, ClientHandler*> HandlerMap;

	const ServicePool& pool_;
    thread_local static HandlerMap handlers_;

public:
	explicit EventDispatcher(const ServicePool& pool) : pool_(pool) {}

	void post_event(const ClientUUID& client, std::unique_ptr<const Event> event) const;
	void post_shared_event(const ClientUUID& client, const std::shared_ptr<const Event>& event) const;
    void register_handler(ClientHandler* handler);
    void remove_handler(ClientHandler* handler);
};

} // ember