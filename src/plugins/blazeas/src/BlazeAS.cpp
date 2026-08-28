/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ember/blaze/Blaze.hpp>
#include <cstdint>

extern "C" {

EMBER_PLUGIN_EXPORT const char* plugin_name = "Blaze Angelscript Extension";
EMBER_PLUGIN_EXPORT const char* plugin_name_short = "blazeas";

EMBER_PLUGIN_EXPORT std::int32_t plugin_on_load() {
	return 0;
}

EMBER_PLUGIN_EXPORT void plugin_on_unload() {

}

} // extern "C"