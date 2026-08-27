/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "BlazeAS.h"
#include "Visibility.h"
#include <ember/blaze/Blaze.h>
#include <angelscript.h>
#include <thread>
#include <memory>
#include <cstdint>
#include <iostream>

std::unique_ptr<std::jthread> runner;

extern "C" {

EMBER_PLUGIN_EXPORT const char* plugin_name = "Blaze Angelscript Extension";
EMBER_PLUGIN_EXPORT const char* plugin_name_short = "blazeas";

EMBER_PLUGIN_EXPORT std::int32_t plugin_on_load() try {
	auto client = std::make_unique<ember::blaze::Client>();

	//runner = std::make_unique<std::jthread>([client = std::move(client)]() mutable {
	//	blazeas::launch(std::move(client));
	//});

	return 0;
} catch(std::exception& e) {
	std::cout << e.what() << std::endl;
	return -1;
}

EMBER_PLUGIN_EXPORT void plugin_on_unload() {

}

} // extern "C"