/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientConnection.h"
#include <memory>
#include <mutex>
#include <unordered_set>

namespace ember {

struct ConnectionStats;

class SessionManager final {
	std::unordered_set<std::unique_ptr<ClientConnection>> sessions_;
	mutable std::mutex sessions_lock_;

public:
	~SessionManager();

	void start(std::unique_ptr<ClientConnection> session);
	void stop(ClientConnection* session);
	void stop_all();
	std::size_t count() const;
	ConnectionStats aggregate_stats() const;
};

} // ember