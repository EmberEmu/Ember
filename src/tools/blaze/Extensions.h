/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#if defined(_WIN32)
#define SHARED_LIBRARY_EXT ".dll"
#elif defined(__linux__)
#define SHARED_LIBRARY_EXT ".so"
#elif defined(__APPLE__) && defined(__MACH__)
#define SHARED_LIBRARY_EXT ".dylib"
#endif