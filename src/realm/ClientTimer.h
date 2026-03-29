/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <chrono>

namespace ember::realm {

class ClientHandler;

class ClientTimer {
	ClientHandler& handler_;

	ClientTimer(ClientHandler& handler)
		: handler_(handler) {}

public:
	void start(const std::chrono::milliseconds& expiry);
	void cancel();

	friend class ClientHandler;
};

} // realm, ember