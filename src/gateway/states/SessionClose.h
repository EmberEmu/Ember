/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ClientContext.h"
#include "../Event.h"
#include <memory>

namespace ember { namespace session_close {

void enter(ClientContext* ctx);
void update(ClientContext* ctx);
void handle_event(ClientContext* ctx, std::shared_ptr<Event> event);
void exit(ClientContext* ctx);

}} // session_close, ember