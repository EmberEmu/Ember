/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FilterTypes.h"
#include "MonitorCallbacks.h"
#include <sstream>

namespace ember {

void monitor_log_callback(const Monitor::Source& source, Monitor::Severity severity,
                          std::intmax_t value, log::Logger& logger) {
	std::stringstream message;
	message << source.key << ":v:" << value << ":t:" << source.threshold << " - ";

	if(source.triggered) {
		message << source.message;
	} else {
		message << "Incident has been resolved.";
	}

	switch(severity) {
		case Monitor::Severity::fatal:
			LOG_FATAL(logger) << message.view() << LOG_ASYNC;
			break;
		case Monitor::Severity::error:
			LOG_ERROR(logger) << message.view() << LOG_ASYNC;
			break;
		case Monitor::Severity::warn:
			LOG_WARN(logger) << message.view() << LOG_ASYNC;
			break;
		case Monitor::Severity::info:
			LOG_INFO(logger) << message.view() << LOG_ASYNC;
			break;
		case Monitor::Severity::debug:
			LOG_DEBUG(logger) << message.view() << LOG_ASYNC;
			break;
	}
}

} // ember