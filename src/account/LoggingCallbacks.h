/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logger.h>
#include <conpool/LogSeverity.h>
#include <string_view>

namespace ember {

inline void pool_log_callback(connection_pool::Severity severity, std::string_view message, log::Logger& logger) {
	constexpr std::string_view fmt = "[pool] {}";

	switch(severity) {
		case connection_pool::Severity::debug:
			LOG_DEBUG_ASYNC(logger, fmt, message);
			break;
		case connection_pool::Severity::info:
			LOG_INFO_ASYNC(logger, fmt, message);
			break;
		case connection_pool::Severity::warn:
			LOG_WARN_ASYNC(logger, fmt, message);
			break;
		case connection_pool::Severity::error:
			LOG_ERROR_ASYNC(logger, fmt, message);
			break;
		case connection_pool::Severity::fatal:
			LOG_FATAL_ASYNC(logger, fmt, message);
			break;
		default:
			LOG_ERROR_ASYNC(logger, "Unhandled connection pool log callback severity");
			LOG_ERROR_ASYNC(logger, fmt, message);
	}
}

} // ember