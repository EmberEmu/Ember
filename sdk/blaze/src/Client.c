/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ember/blaze/Client.h>
#include <stdlib.h>

struct Client* client_create() {
	struct Client* client = calloc(1, sizeof(struct Client));

	if(!client) {
		return NULL;
	}

	client->state = cs_created;
	return client;
}

void client_close(struct Client* client) {
	client->state = cs_closed;
}

void client_destroy(struct Client* client) {
	free(client);
}