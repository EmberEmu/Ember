/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/unordered/unordered_flat_map.hpp>

namespace ember::gateway {

class WorldConnection;

class WorldSessions final {
	static inline thread_local std::unordered_map<unsigned int, WorldConnection*> connections_;

public:
	void insert(unsigned int map_id, WorldConnection* connection);
	void erase(unsigned int map_id);
	void erase(const WorldConnection* connection);
	WorldConnection* find(unsigned int map_id) const;
};

} // gateway, ember