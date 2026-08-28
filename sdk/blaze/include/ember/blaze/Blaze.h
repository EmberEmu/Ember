/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ember/blaze/Common.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

EMBER_PLUGIN_EXPORT SDKBuildMeta sdk_build();
EMBER_PLUGIN_EXPORT uint8_t sdk_initialise(BlazeHostAPI api);

void log_test_0(const char* message, uint32_t size);

#ifdef __cplusplus
} // extern "C"
#endif
