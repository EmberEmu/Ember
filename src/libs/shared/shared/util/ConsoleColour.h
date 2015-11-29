/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember { namespace util {

enum class Colour : unsigned int {
	BLACK, BLUE, GREEN, CYAN, RED, MAGENTA,
	BROWN, GREY, DARK_GREY, LIGHT_BLUE, LIGHT_GREEN,
	LIGHT_CYAN, LIGHT_RED, LIGHT_MAGENTA, YELLOW, WHITE,
	DEFAULT
};

void set_output_colour(Colour colour);
Colour save_output_colour();

}} // util, ember