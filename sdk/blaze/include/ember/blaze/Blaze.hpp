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
#include <cstdint>

namespace ember::blaze {

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

} // blaze, ember