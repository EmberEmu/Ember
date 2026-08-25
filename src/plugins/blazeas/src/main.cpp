/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ember/blaze/dummy.h>
#include <angelscript.h>
#include <iostream>

#ifdef _WIN32

#include <Windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ulReason, LPVOID lpReserved) {
	if(ulReason == DLL_PROCESS_ATTACH) {
		std::cout << "Hello, world!\n";
	}

	if(ulReason == DLL_PROCESS_DETACH) {
		std::cout << "Farewell, world!\n";
	}

	return TRUE;
}

#endif