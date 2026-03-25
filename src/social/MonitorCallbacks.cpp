/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FilterTypes.h"
#include "MonitorCallbacks.h"
#include <format>

namespace ember {

void monitor_log_callback(const Monitor::Source& source, Monitor::Severity severity,
                          std::intmax_t value, log::Logger& logger) {
	auto message = std::format(
		"{}:v:{}:t:{} - {}", source.key, value, source.threshold,
		source.triggered? source.message : "Incident has been resolved."
	);

	switch(severity) {
		case Monitor::Severity::fatal:
			LOG_FATAL(logger, message);
			break;
		case Monitor::Severity::error:
			LOG_ERROR(logger, message);
			break;
		case Monitor::Severity::warn:
			LOG_WARN(logger, message);
			break;
		case Monitor::Severity::info:
			LOG_INFO(logger, message);
			break;
		case Monitor::Severity::debug:
			LOG_DEBUG(logger, message);
			break;
	}
}

} // ember