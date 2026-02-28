/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WorldSessions.h"
#include "WorldConnection.h"

namespace ember::gateway {

void WorldSessions::insert(unsigned int map_id, WorldConnection* connection) {

}

void WorldSessions::erase(unsigned int map_id) {

}

void WorldSessions::erase(const WorldConnection* connection) {

}

WorldConnection* WorldSessions::find(unsigned int map_id) const {
	return nullptr;
}


} // gateway, ember