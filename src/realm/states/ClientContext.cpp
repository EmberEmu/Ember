/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "../ClientHandler.h"

namespace ember::realm {

void ClientContext::start_timer(std::chrono::milliseconds time) {
	handler.start_timer(time);
}

void ClientContext::cancel_timer() {
	handler.cancel_timer();
}

} // realm, ember