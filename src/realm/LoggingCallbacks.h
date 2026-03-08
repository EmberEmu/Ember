/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logger.h>
#include <stun/Logging.h>
#include <conpool/LogSeverity.h>
#include <ports/LogSeverity.h>
#include <string_view>

namespace ember {

inline void forward_log_callback(ports::Severity severity, const std::string_view message, log::Logger& logger) {
	constexpr std::string_view fmt = "[ports] {}";

	switch(severity) {
		case ports::Severity::debug:
			LOG_DEBUG_ASYNC(logger, fmt, message);
			break;
		case ports::Severity::info:
			LOG_INFO_ASYNC(logger, fmt, message);
			break;
		case ports::Severity::warn:
			LOG_WARN_ASYNC(logger, fmt, message);
			break;
		case ports::Severity::error:
			LOG_ERROR_ASYNC(logger, fmt, message);
			break;
		case ports::Severity::fatal:
			LOG_FATAL_ASYNC(logger, fmt, message);
			break;
		default:
			LOG_ERROR_ASYNC(logger, "Unhandled port forward log callback severity");
			LOG_ERROR_ASYNC(logger, fmt, message);
	}
}

inline static void stun_log_callback(stun::Severity severity, stun::Error reason, log::Logger& logger) {
	constexpr std::string_view fmt = "[stun] {}";
	const std::string_view reason_str = to_string(reason);

	switch(severity) {
		case stun::Severity::trace:
			LOG_TRACE_SYNC(logger, fmt, reason_str);
			break;
		case stun::Severity::debug:
			LOG_DEBUG_SYNC(logger, fmt, reason_str);
			break;
		case stun::Severity::info:
			LOG_INFO_SYNC(logger, fmt, reason_str);
			break;
		case stun::Severity::warn:
			LOG_WARN_SYNC(logger, fmt, reason_str);
			break;
		case stun::Severity::error:
			LOG_ERROR_SYNC(logger, fmt, reason_str);
			break;
		case stun::Severity::fatal:
			LOG_FATAL_SYNC(logger, fmt, reason_str);
			break;
		default:
			LOG_ERROR_ASYNC(logger, "Unhandled STUN log callback severity");
			LOG_ERROR_SYNC(logger, fmt, reason_str);
	}
}

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