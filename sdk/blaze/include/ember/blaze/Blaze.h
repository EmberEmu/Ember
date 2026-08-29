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

void blaze_log(uint8_t log_level, const char* message);
void blaze_slog(uint8_t log_level, const char* message);
void blaze_log_sstr(uint8_t log_level, const SizedString name);
void blaze_slog_sstr(uint8_t log_level, const SizedString name);

void blaze_command_create(const char* name, const char* description);
void blaze_command_create_sstr(const SizedString name, const SizedString description);
bool blaze_command_destroy(void* command);
bool blaze_command_add_argument(void* command, const char* name, uint8_t type, bool required);
bool blaze_command_add_argument_sstr(void* command, const SizedString name, uint8_t type, bool required);
bool blaze_command_callback(void* command);

PluginID blaze_get_plugin_id();

#ifdef __cplusplus
} // extern "C"
#endif
