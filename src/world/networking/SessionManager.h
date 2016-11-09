/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <cstddef>

namespace ember {

class GatewayClient;

class SessionManager final {
	std::set<std::shared_ptr<GatewayClient>> sessions_;
	mutable std::mutex sessions_lock_;

public:
	~SessionManager();

	void start(std::shared_ptr<GatewayClient> session);
	void stop(GatewayClient* session);
	void stop_all();
};

} // ember