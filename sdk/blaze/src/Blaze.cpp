/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ember/blaze/Blaze.h>
#include <iostream>

static BlazeHostAPI host_api;

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

uint8_t sdk_initialise(BlazeHostAPI api) {
	if(api.size != sizeof(BlazeHostAPI)) {
		return SDK_INIT_BAD_SIZE;
	}

	if(api.version_major != SDK_MAJOR_VERSION) {
		return SDK_INIT_BAD_VERS;
	}

	host_api = api;
	(*api.log_sync)("Hello from the plugin", LOG_LEVEL_INFO);
	return SDK_INIT_OK;
}

void log_test_0(const char* message, const uint32_t size) {
	(*host_api.log_sync)(message, LOG_LEVEL_INFO);
}