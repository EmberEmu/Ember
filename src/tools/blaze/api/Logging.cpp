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

void log_async(std::uint8_t level, const CountedString* message, PluginID pid) {
	std::string_view view(message->data, message->size);
	const auto fmt = std::format("[{}] {}", pid, view);

	auto logger = ember::log::global_logger(); // todo, temp!

	switch(level) {
		case LOG_LEVEL_TRACE:
			LOG_TRACE(logger, fmt);
			break;
		case LOG_LEVEL_DEBUG:
			LOG_DEBUG(logger, fmt);
			break;
		case LOG_LEVEL_INFO:
			LOG_INFO(logger, fmt);
			break;
		case LOG_LEVEL_WARN:
			LOG_WARN(logger, fmt);
			break;
		case LOG_LEVEL_ERROR:
			LOG_ERROR(logger, fmt);
			break;
		case LOG_LEVEL_FATAL:
			LOG_FATAL(logger, fmt);
			break;
		default:
			LOG_ERROR(logger, "Bad logging level, message: {}", fmt);
	}
}

void log_sync(std::uint8_t level, const CountedString* message, PluginID pid) {
	std::string_view view(message->data, message->size);
	const auto fmt = std::format("[{}] {}", pid, view);

	auto logger = ember::log::global_logger(); // todo, temp!

	switch(level) {
		case LOG_LEVEL_TRACE:
			SLOG_TRACE(logger, fmt);
			break;
		case LOG_LEVEL_DEBUG:
			SLOG_DEBUG(logger, fmt);
			break;
		case LOG_LEVEL_INFO:
			SLOG_INFO(logger, fmt);
			break;
		case LOG_LEVEL_WARN:
			SLOG_WARN(logger, fmt);
			break;
		case LOG_LEVEL_ERROR:
			SLOG_ERROR(logger, fmt);
			break;
		case LOG_LEVEL_FATAL:
			SLOG_FATAL(logger, fmt);
			break;
		default:
			SLOG_ERROR(logger, "Bad logging level, message: {}", fmt);
	}
}