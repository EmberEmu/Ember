/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "BlazeAS.h"
#include <cstdint>

extern "C" {

EMBER_PLUGIN_EXPORT const char* plugin_name = "Blaze Angelscript Extension";
EMBER_PLUGIN_EXPORT const char* plugin_name_short = "blazeas";

} // extern "C"

namespace blazeas {

void Plugin::on_load() {
	ember::blaze::log_test("This is just an ABI test, disregard");
}

void Plugin::on_unload() {

}

} // blazeas