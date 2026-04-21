/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef EMBER_DEBUG_ALLOCATOR_TRACKING
#include <allocators/Tracking.h>
#define ALLOC_TRACK(tag, type, ...) \
ember::allocators::tracking::record(tag, type __VA_OPT__(,) __VA_ARGS__);
#else
#define ALLOC_TRACK(tag, type, ...)
#endif