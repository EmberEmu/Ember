/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "InQueue.h"
#include "../ClientHandler.h"
#include "../ClientConnection.h"
#include "../temp.h"

namespace ember { namespace queue {

void enter(ClientContext* ctx) {
	// don't care
}

void update(ClientContext* ctx) {
	// don't care
}

void exit(ClientContext* ctx) {
	queue_service_temp->dequeue(ctx->connection->shared_from_this());
}

}} // queue, ember