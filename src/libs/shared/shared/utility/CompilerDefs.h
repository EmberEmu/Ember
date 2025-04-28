/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#if defined(__clang__) || defined(__GNUC__)
	#define MUST_TAIL [[clang::musttail]]
#else
	#define MUST_TAIL
#endif