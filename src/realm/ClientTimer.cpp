/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientTimer.h"
#include "ClientHandler.h"

namespace ember::realm {

void ClientTimer::start(const std::chrono::milliseconds& expiry) {
	handler_.start_timer(expiry);
}

void ClientTimer::cancel() {
	handler_.cancel_timer();
}

} // realm, ember