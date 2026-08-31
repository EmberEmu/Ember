/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#if defined _WIN32 || defined(__CYGWIN__)
	#ifdef BUILD_SHARED_SERVICES
		#ifdef EXPORT_SERVICE
			#define EMBER_EXPORT_SERVICE __declspec(dllexport)
		#else
			#define EMBER_EXPORT_SERVICE __declspec(dllimport)
		#endif // EXPORT_SERVICE
	#else
		#define EMBER_EXPORT_SERVICE
	#endif // BUILD_SHARED_SERVICES
#elif defined(__GNUC__) || defined(__clang__)
	#define EMBER_EXPORT_SERVICE __attribute__((visibility("default")))
#else
	#define EMBER_EXPORT_SERVICE
#endif