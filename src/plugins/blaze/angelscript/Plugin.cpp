/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Plugin.h"
#include <cstdint>

using namespace ember;

namespace blazeas {

extern "C" {

EMBER_EXPORT const char* plugin_name = "Blaze Angelscript Extension";
EMBER_EXPORT const char* plugin_name_short = "blazeas";

} // extern "C"

Plugin::Plugin() {
	blaze::log(blaze::LogLevel::info, "Hello from plugin {}", blaze::plugin_id());
}

Plugin::~Plugin() {
	blaze::log(blaze::LogLevel::info, "Farewell!");
}

} // blazeas