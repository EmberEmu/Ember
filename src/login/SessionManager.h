/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <cstdint>

namespace ember {

class NetworkSession;

class SessionManager {
	std::set<std::shared_ptr<NetworkSession>> sessions_;
	std::mutex sessions_lock_;

public:
	void start(std::shared_ptr<NetworkSession> session);
	void stop(std::shared_ptr<NetworkSession> session);
	void stop_all();
	std::size_t count() const;
};

} // ember