/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Logging.h"
#include <logger/Logger.h>

enum log_level {
	ll_trace = 0,
	ll_debug = 1,
	ll_info  = 2,
	ll_warn  = 3,
	ll_error = 4
};

void log_async(const char* message, std::uint8_t level) {
	auto logger = ember::log::global_logger(); // todo, temp!

	switch(level) {
		case ll_trace:
			LOG_TRACE(logger, message);
			break;
		case ll_debug:
			LOG_DEBUG(logger, message);
			break;
		case ll_info:
			LOG_INFO(logger, message);
			break;
		case ll_warn:
			LOG_WARN(logger, message);
			break;
		case ll_error:
			LOG_ERROR(logger, message);
			break;
		default:
			LOG_ERROR(logger, "Bad logging level, message: {}", message);
	}
}

void log_sync(const char* message, std::uint8_t level) {
	auto logger = ember::log::global_logger(); // todo, temp!

	switch(level) {
		case ll_trace:
			SLOG_TRACE(logger, message);
			break;
		case ll_debug:
			SLOG_DEBUG(logger, message);
			break;
		case ll_info:
			SLOG_INFO(logger, message);
			break;
		case ll_warn:
			SLOG_WARN(logger, message);
			break;
		case ll_error:
			SLOG_ERROR(logger, message);
			break;
		default:
			SLOG_ERROR(logger, "Bad logging level, message: {}", message);
	}
}

