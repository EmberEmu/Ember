/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ember/blaze/Client.h>
#include <stdlib.h>

struct blaze_client* blaze_client_create() {
	struct blaze_client* client = calloc(1, sizeof(struct blaze_client));

	if(!client) {
		return NULL;
	}

	client->state = cs_created;
	return client;
}

void blaze_client_close(struct blaze_client* client) {
	client->state = cs_closed;
}

void blaze_client_destroy(struct blaze_client* client) {
	free(client);
}