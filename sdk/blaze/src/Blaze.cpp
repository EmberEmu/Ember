/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ember/blaze/Blaze.h>

static BlazeHostAPI host_api;
static PluginID plugin_id;

SDKBuildMeta sdk_build() {
	return {
		.magic = SDK_MAGIC,
		.sdk_init_meta_size = sizeof(SDKBuildMeta),
		.blaze_host_api_size = sizeof(BlazeHostAPI),
		.version_major = SDK_MAJOR_VERSION,
		.version_minor = SDK_MINOR_VERSION,
		.version_patch = SDK_PATCH_VERSION
	};
}

uint8_t sdk_initialise(const BlazeHostAPI api, const uint64_t pid) {
	if(api.size != sizeof(BlazeHostAPI)) {
		return SDK_INIT_BAD_SIZE;
	}

	if(api.version_major != SDK_MAJOR_VERSION) {
		return SDK_INIT_BAD_VERS;
	}

	host_api = api;
	plugin_id = pid;
	return SDK_INIT_OK;
}

PluginID get_plugin_id() {
	return plugin_id;
}

void log_test_0(const char* message, const uint32_t size, uint8_t log_level) {
	(*host_api.log_sync)(message, size, log_level, get_plugin_id());
}