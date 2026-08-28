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
	return SDKBuildMeta {
		.magic = SDK_MAGIC,
		.sdk_init_meta_size = sizeof(SDKBuildMeta),
		.blaze_host_api_size = sizeof(BlazeHostAPI),
		.version_major = SDK_MAJOR_VERSION,
		.version_minor = SDK_MINOR_VERSION,
		.version_patch = SDK_PATCH_VERSION
	};
}

void sdk_initialise(BlazeHostAPI api) {
	host_api = api;
}

void test() {
	std::cout << "Hello, world!";
}