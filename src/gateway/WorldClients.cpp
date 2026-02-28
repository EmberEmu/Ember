/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WorldClients.h"

namespace ember::gateway {

void WorldClients::insert(boost::uuids::uuid uuid, ClientConnection* connection) {
	connections_.insert_or_assign(uuid, connection);
}

void WorldClients::erase(boost::uuids::uuid uuid) {
	connections_.erase(uuid);
}

ClientConnection* WorldClients::find(boost::uuids::uuid uuid) const {
	if(auto it = connections_.find(uuid); it != connections_.end()) {
		return it->second;
	}

	return nullptr;
}

} // gateway, ember