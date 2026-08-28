/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "../Common.h"
#include "Logging.h"
#include <logger/Logger.h>

void log_async(const char* message, std::uint8_t level) {
	auto logger = ember::log::global_logger(); // todo, temp!

	switch(level) {
		case LOG_LEVEL_TRACE:
			LOG_TRACE(logger, message);
			break;
		case LOG_LEVEL_DEBUG:
			LOG_DEBUG(logger, message);
			break;
		case LOG_LEVEL_INFO:
			LOG_INFO(logger, message);
			break;
		case LOG_LEVEL_WARN:
			LOG_WARN(logger, message);
			break;
		case LOG_LEVEL_ERROR:
			LOG_ERROR(logger, message);
			break;
		case LOG_LEVEL_FATAL:
			LOG_FATAL(logger, message);
			break;
		default:
			LOG_ERROR(logger, "Bad logging level, message: {}", message);
	}
}

void log_sync(const char* message, std::uint8_t level) {
	auto logger = ember::log::global_logger(); // todo, temp!

	switch(level) {
		case LOG_LEVEL_TRACE:
			SLOG_TRACE(logger, message);
			break;
		case LOG_LEVEL_DEBUG:
			SLOG_DEBUG(logger, message);
			break;
		case LOG_LEVEL_INFO:
			SLOG_INFO(logger, message);
			break;
		case LOG_LEVEL_WARN:
			SLOG_WARN(logger, message);
			break;
		case LOG_LEVEL_ERROR:
			SLOG_ERROR(logger, message);
			break;
		case LOG_LEVEL_FATAL:
			SLOG_FATAL(logger, message);
			break;
		default:
			SLOG_ERROR(logger, "Bad logging level, message: {}", message);
	}
}