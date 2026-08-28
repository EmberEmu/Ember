/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ember/blaze/Common.h>
#include <ember/blaze/Blaze.h>
#include <memory>
#include <string>
#include <string_view>
#include <cstdint>

namespace ember::blaze {

#define SDK_REGISTER_PLUGIN(PluginType)                    \
std::unique_ptr<PluginType> plugin_inst;                   \
                                                           \
extern "C" {                                               \
	inline EMBER_PLUGIN_EXPORT void plugin_on_load() {     \
		plugin_inst->on_load();                            \
	}                                                      \
	inline EMBER_PLUGIN_EXPORT void plugin_on_unload() {   \
		if(plugin_inst) {                                  \
			plugin_inst->on_unload();                      \
		}                                                  \
	}                                                      \
}

class Plugin {
public:
	virtual void on_load() = 0;
	virtual void on_unload() = 0;
	virtual ~Plugin() = default;
};

enum class LogLevel : std::uint8_t {
	trace = LOG_LEVEL_TRACE,
	debug = LOG_LEVEL_DEBUG,
	info  = LOG_LEVEL_INFO,
	warn  = LOG_LEVEL_WARN,
	error = LOG_LEVEL_ERROR,
	fatal = LOG_LEVEL_FATAL
};

enum class ArgumentType : std::uint8_t {
	string  = CAT_STRING,
	float32 = CAT_FLOAT,
	float64 = CAT_DOUBLE,
	int8    = CAT_INT_8,
	int16   = CAT_INT_16,
	int32   = CAT_INT_32,
	int64   = CAT_INT_64,
	uint8   = CAT_UINT_8,
	uint16  = CAT_UINT_16,
	uint32  = CAT_UINT_32,
	uint64  = CAT_UINT_64
};

inline void log_test(std::string_view message) {
	log_test_0(message.data(), message.size());
}

} // blaze, ember