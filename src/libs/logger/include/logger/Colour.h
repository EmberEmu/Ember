/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::log {

enum class Colour : unsigned int {
	black,
	blue,
	green,
	cyan,
	red,
	magenta,
	brown,
	grey,
	dark_grey,
	light_blue,
	light_green,
	light_cyan,
	light_red,
	light_magenta,
	yellow, white,
	white_on_red_bg,
	black_on_white_bg,
	white_on_grey_bg,
	white_on_cyan_bg,
	default_colour
};

} // log, ember