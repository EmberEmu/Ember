/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Visibility.h"
#include <logger/LoggerFwd.h>
#include <cstdint>

namespace ember::log {

class Logger;

} // log, ember

extern "C" {

struct logger {
	ember::log::Logger* impl;
};

EMBER_EXPORT logger log_get_logger();
EMBER_EXPORT void log_async(logger logger, const char* message, std::uint8_t level);
EMBER_EXPORT void log_sync(logger logger, const char* message, std::uint8_t level);

}