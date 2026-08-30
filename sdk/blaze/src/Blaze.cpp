/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ember/blaze/Blaze.h>
#include <cstring>

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

uint8_t sdk_initialise(const BlazeHostAPI api, const PluginID pid) {
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

CountedString to_counted_string(const char* string) {
	return {
		.data = string,
		.size = static_cast<uint64_t>(std::strlen(string))
	};
}

PluginID blaze_get_plugin_id() {
	return plugin_id;
}

void blaze_log(const uint8_t log_level, const char* message) {
	const auto sstr = to_counted_string(message);
	(*host_api.log_async)(log_level, &sstr, blaze_get_plugin_id());
}

void blaze_slog(const uint8_t log_level, const char* message) {
	const auto sstr = to_counted_string(message);
	(*host_api.log_sync)(log_level, &sstr, blaze_get_plugin_id());
}

void blaze_log_sstr(const uint8_t log_level, const CountedString message) {
	(*host_api.log_async)(log_level, &message, blaze_get_plugin_id());
}

void blaze_slog_sstr(const uint8_t log_level, const CountedString message) {
	(*host_api.log_sync)(log_level, &message, blaze_get_plugin_id());
}

void blaze_command_create(const char* name, const char* description) {
	const auto name_sstr = to_counted_string(name);
	const auto desc_sstr = to_counted_string(description);
	(*host_api.command_create)(&name_sstr, &desc_sstr);
}

void blaze_command_create_sstr(const CountedString name, const CountedString description) {
	(*host_api.command_create)(&name, &description);
}

bool blaze_command_destroy(void* command) {
	(*host_api.command_destroy)(command);
}

bool blaze_command_add_argument(void* command, const char* name,
                                const uint8_t type, const bool required) {
	const auto sstr = to_counted_string(name);
	(*host_api.command_add_argument)(command, &sstr, type, required);
}

bool blaze_command_add_argument_sstr(void* command, const CountedString name,
                                const uint8_t type, const bool required) {
	(*host_api.command_add_argument)(command, &name, type, required);
}

bool blaze_command_callback(void* command) {
	(*host_api.command_callback)(command);
}