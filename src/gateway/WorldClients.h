/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientConnection.h"
#include <boost/uuid/uuid.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <memory>

namespace ember::gateway {

class ClientConnection;

class WorldClients final {
	static inline thread_local boost::unordered_flat_map<boost::uuids::uuid, ClientConnection*> tls_connections;
	boost::unordered_flat_map<boost::uuids::uuid, ClientConnection*>& connections;

public:
	WorldClients()
		: connections(tls_connections) {}

	void insert(boost::uuids::uuid uuid, ClientConnection* connection);
	void erase(boost::uuids::uuid uuid);
	ClientConnection* find(boost::uuids::uuid uuid);
};

} // gateway, ember