/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// todo, proper compiler/platform detection
#ifdef _WIN32
	#define EMBER_EXPORT __declspec(dllexport)
#else
	#define EMBER_EXPORT __attribute__((visibility("default")))
#endif // _WIN32

#define SDK_MAGIC          0x424c5a45 // 'BLZE', endian test
#define SDK_INIT_OK        0
#define SDK_INIT_BAD_SIZE  1
#define SDK_INIT_BAD_VERS  2

#define SDK_MAJOR_VERSION 1
#define SDK_MINOR_VERSION 0
#define SDK_PATCH_VERSION 0

typedef uint8_t  LogLevel;
typedef uint8_t  ArgumentType;
typedef uint64_t PluginID;

// logging severity levels
#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_FATAL 5

// command argument types
#define CAT_CHAR   0
#define CAT_STRING 1
#define CAT_FLOAT  2
#define CAT_DOUBLE 3
#define CAT_INT8   4
#define CAT_INT16  5
#define CAT_INT32  6
#define CAT_INT64  7
#define CAT_UINT8  8
#define CAT_UINT16 9
#define CAT_UINT32 10
#define CAT_UINT64 11

typedef struct {
	const char* data;
	uint64_t size;
} CountedString;

// plugin API
typedef void(*plugin_load_fn)();
typedef void(*plugin_unload_fn)();

// logging API
typedef void(*log_async_fn)(LogLevel level, const CountedString* message, PluginID pid);
typedef void(*log_sync_fn)(LogLevel level, const CountedString* message, PluginID pid);

// command API - todo, opaque struct once I've cleaned the other files up
typedef void*(*command_create_fn)(const CountedString* name, const CountedString* description);
typedef bool(*command_destroy_fn)(void* command);
typedef bool(*command_add_argument_fn)(void* command, const CountedString* name, ArgumentType type, bool required);
typedef bool(*command_callback_fn)(void* command);

typedef struct {
	uint16_t size;
	uint16_t version_major;
	uint16_t version_minor;
	uint16_t version_patch;

	log_async_fn log_async;
	log_sync_fn log_sync;

	command_create_fn command_create;
	command_destroy_fn command_destroy;
	command_add_argument_fn command_add_argument;
	command_callback_fn command_callback;
} HostAPI;

typedef struct {
	uint32_t magic;
	uint16_t sdk_init_meta_size;
	uint16_t blaze_host_api_size;
	uint16_t version_major;
	uint16_t version_minor;
	uint16_t version_patch;
} SDKBuildMeta;

// SDK internals
typedef SDKBuildMeta(*sdk_build_fn)();
typedef uint8_t(*sdk_initialise_fn)(HostAPI api, PluginID plugin_id);

#ifdef __cplusplus
} // extern "C"
#endif