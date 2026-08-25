/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef BLAZE_SDK_CLIENT_H
#define BLAZE_SDK_CLIENT_H

#include <ember/blaze/ClientState.h>
#include <ember/blaze/Config.h>

struct blaze_client {
	enum blaze_client_state state;
};

EMBER_EXPORT_SDK struct blaze_client* blaze_client_create();
EMBER_EXPORT_SDK void blaze_client_destroy(struct blaze_client* client);

#endif // BLAZE_SDK_CLIENT_H