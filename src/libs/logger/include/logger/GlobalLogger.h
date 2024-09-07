/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/LoggerFwd.h>

namespace ember::log {

namespace detail {

extern Logger* logger_;

} // detail

Logger* global_logger();
void global_logger(Logger& logger);
void global_logger(Logger* logger);

} // log, ember