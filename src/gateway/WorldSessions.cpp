/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WorldSessions.h"
#include "WorldConnection.h"

namespace ember {

void WorldSessions::add_world(WorldID id, std::shared_ptr<WorldConnection> connection) {

}

void WorldSessions::remove_world(WorldID id) {

}

void WorldSessions::remove_world(std::shared_ptr<WorldConnection> connection) {

}

std::shared_ptr<WorldConnection> WorldSessions::locate_world(WorldID id) {
	return nullptr;
}


} // ember