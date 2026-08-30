/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "../Common.h"
#include "../InterfaceContainer.h"
#include "Logging.h"
#include <logger/Logger.h>
#include <format>
#include <string_view>
#include <cassert>

namespace ember::blaze {

namespace {

std::string format_message(const CountedString* message, const PluginID pid) {
	auto registry = InterfaceContainer::get_instance().plugin_registry();
	assert(registry);
	const auto plugin = registry->locate(pid);
	std::string_view view(message->data, message->size);
	return std::format("[{}] {}", plugin? plugin->name_short() : "unknown", view);
}

} // unnamed


void log_async(std::uint8_t level, const CountedString* message, const PluginID pid) {
	auto logger = InterfaceContainer::get_instance().logger();
	const auto formatted = format_message(message, pid);

	switch(level) {
		case LOG_LEVEL_TRACE:
			LOG_TRACE(logger, formatted);
			break;
		case LOG_LEVEL_DEBUG:
			LOG_DEBUG(logger, formatted);
			break;
		case LOG_LEVEL_INFO:
			LOG_INFO(logger, formatted);
			break;
		case LOG_LEVEL_WARN:
			LOG_WARN(logger, formatted);
			break;
		case LOG_LEVEL_ERROR:
			LOG_ERROR(logger, formatted);
			break;
		case LOG_LEVEL_FATAL:
			LOG_FATAL(logger, formatted);
			break;
		default:
			LOG_ERROR(logger, "Bad logging level, message: {}", formatted);
	}
}

void log_sync(std::uint8_t level, const CountedString* message, const PluginID pid) {
	auto logger = InterfaceContainer::get_instance().logger();
	const auto formatted = format_message(message, pid);

	switch(level) {
		case LOG_LEVEL_TRACE:
			SLOG_TRACE(logger, formatted);
			break;
		case LOG_LEVEL_DEBUG:
			SLOG_DEBUG(logger, formatted);
			break;
		case LOG_LEVEL_INFO:
			SLOG_INFO(logger, formatted);
			break;
		case LOG_LEVEL_WARN:
			SLOG_WARN(logger, formatted);
			break;
		case LOG_LEVEL_ERROR:
			SLOG_ERROR(logger, formatted);
			break;
		case LOG_LEVEL_FATAL:
			SLOG_FATAL(logger, formatted);
			break;
		default:
			SLOG_ERROR(logger, "Bad logging level, message: {}", formatted);
	}
}

} // blaze, ember