/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

typedef struct logger* logger_t;

using log_get_logger = logger(*)();
using log_async = void(*)(logger logger, const char* message, std::uint8_t level);
using log_sync = void(*)(logger logger, const char* message, std::uint8_t level);