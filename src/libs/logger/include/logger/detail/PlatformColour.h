/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Colour.h>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#endif

#include <string_view>

namespace ember::log::detail {

#ifdef _WIN32
WORD colour_attribute(Colour colour);
#endif

std::string_view ansi_sequence(Colour colour);

} // detail, log, ember