/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::utility {

enum class Colour : unsigned int {
	BLACK, BLUE, GREEN, CYAN, RED, MAGENTA,
	BROWN, GREY, DARK_GREY, LIGHT_BLUE, LIGHT_GREEN,
	LIGHT_CYAN, LIGHT_RED, LIGHT_MAGENTA, YELLOW, WHITE,
	WHITE_ON_RED_BG, BLACK_ON_WHITE_BG, WHITE_ON_GREY_BG,
	WHITE_ON_CYAN_BG, DEFAULT
};

void set_console_out_colour(Colour colour);
Colour save_console_out_colour();

class ConsoleColour final {
	Colour original_;

public:
	ConsoleColour(Colour colour) : original_(save_console_out_colour()) {
		set_console_out_colour(colour);
	}

	ConsoleColour() : original_(save_console_out_colour()) {}

	~ConsoleColour() {
		reset();
	}

	void set(Colour colour) {
		set_console_out_colour(colour);
	}

	void reset() {
		set_console_out_colour(original_);
	}
};

} // util, ember