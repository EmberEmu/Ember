/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WorldClients.h"

namespace ember::realm {

void WorldClients::insert(boost::uuids::uuid uuid, ClientConnection* connection) {
	connections.insert_or_assign(uuid, connection);
}

void WorldClients::erase(boost::uuids::uuid uuid) {
	connections.erase(uuid);
}

ClientConnection* WorldClients::find(boost::uuids::uuid uuid) {
	if(auto it = connections.find(uuid); it != connections.end()) {
		return it->second;
	}

	return nullptr;
}

} // realm, ember