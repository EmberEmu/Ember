/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FilterTypes.h"
#include "MonitorCallbacks.h"
#include "NetworkListener.h"
#include <format>

namespace ember {

using namespace std::chrono_literals;

void install_net_monitor(Monitor& monitor, const NetworkListener& server, log::Logger& logger) {
	Monitor::Source source{ "network_connections", [&] { return server.connection_count(); },
		10s, 1000,
		[](std::intmax_t value, std::intmax_t threshold) {
			return value > threshold;
		},
		"High concurrent connection count!",
	};

	monitor.add_source(source, Monitor::Severity::warn,
		[&](const auto& source, auto severity, auto value) {
			monitor_log_callback(source, severity, value, logger);
		}
	);

}

void monitor_log_callback(const Monitor::Source& source, Monitor::Severity severity,
                          std::intmax_t value, log::Logger& logger) {
	auto message = std::format(
		"{}:v:{}:t:{} - {}", source.key, value, source.threshold,
		source.triggered? source.message : "Incident has been resolved."
	);

	switch(severity) {
		case Monitor::Severity::fatal:
			LOG_FATAL(logger) << message << LOG_ASYNC;
			break;
		case Monitor::Severity::error:
			LOG_ERROR(logger) << message << LOG_ASYNC;
			break;
		case Monitor::Severity::warn:
			LOG_WARN(logger) << message << LOG_ASYNC;
			break;
		case Monitor::Severity::info:
			LOG_INFO(logger) << message << LOG_ASYNC;
			break;
		case Monitor::Severity::debug:
			LOG_DEBUG(logger) << message << LOG_ASYNC;
			break;
	}
}

} // ember