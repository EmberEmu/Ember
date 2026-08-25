/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef _WIN32
	#define EMBER_PLUGIN_EXPORT __declspec(dllexport) 
#else
	#define EMBER_PLUGIN_EXPORT
#endif // _WIN32