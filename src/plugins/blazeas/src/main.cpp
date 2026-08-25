/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Config.h"
#include <ember/blaze/BlazeSDK.h>
#include <angelscript.h>
#include <iostream>

extern "C" {

EMBER_PLUGIN_EXPORT const char* plugin_name = "Blaze Angelscript Runner";

}

#ifdef _WIN32

#include <Windows.h>

Client* client = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ulReason, LPVOID lpReserved) {
	if(ulReason == DLL_PROCESS_ATTACH) {
		client = client_create();
		std::cout << "Client created\n";
	}

	if(ulReason == DLL_PROCESS_DETACH) {
		client_destroy(client);
		std::cout << "Client destroyed\n";
	}

	return TRUE;
}

#endif