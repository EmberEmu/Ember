/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef BLAZE_SDK_CLIENT_STATE_H
#define BLAZE_SDK_CLIENT_STATE_H

enum ClientState {
	cs_created,
	cs_connected,
	cs_logged_in,
	cs_disconnected,
	cs_destroyed
};

#endif // BLAZE_SDK_CLIENT_STATE_H