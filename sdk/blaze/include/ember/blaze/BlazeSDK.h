/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef BLAZE_SDK_BLAZE_H
#define BLAZE_SDK_BLAZE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ember/blaze/Client.h>

extern void* handle;

int blaze_sdk_init();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BLAZE_SDK_BLAZE_H