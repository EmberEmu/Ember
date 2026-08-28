/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "../Common.h"
#include <cstdint>

extern "C" {

EMBER_EXPORT void log_async(const SizedString* message, std::uint8_t level, PluginID pid);
EMBER_EXPORT void log_sync(const SizedString* message, std::uint8_t level, PluginID pid);

} // extern "C"