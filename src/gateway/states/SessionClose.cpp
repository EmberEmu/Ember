/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SessionClose.h"

namespace ember { namespace session_close {

void enter(ClientContext* ctx) {
	// don't care, for now
}

void update(ClientContext* ctx) {
	// don't care, for now
}

void handle_event(ClientContext* ctx, std::shared_ptr<Event> event) {
    // don't care, for now
}

void exit(ClientContext* ctx) {
	// don't care, for now
}

}} // session_close, ember