/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/GlobalLogger.h>
#include <logger/Logger.h>

namespace ember::log {

namespace detail {

Logger* logger_;

} // detail

Logger* global_logger() {
	return detail::logger_;
}

void global_logger(Logger& logger) {
	global_logger(&logger);
}

void global_logger(Logger* logger) {
	detail::logger_ = logger;
}

} // log, ember