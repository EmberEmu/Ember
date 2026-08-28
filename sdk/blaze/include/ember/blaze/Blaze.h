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

EMBER_EXPORT SDKBuildMeta sdk_build();
EMBER_EXPORT uint8_t sdk_initialise(BlazeHostAPI api, PluginID pid);

void log_test_0(const char* message, uint32_t size, uint8_t log_level);
PluginID get_plugin_id();

#ifdef __cplusplus
} // extern "C"
#endif
