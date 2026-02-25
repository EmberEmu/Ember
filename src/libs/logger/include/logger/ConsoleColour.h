/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::log {

enum class Colour : unsigned int {
	black, blue, green, cyan, red, magenta,
	brown, grey, dark_grey, light_blue, light_green,
	light_cyan, light_red, light_magenta, yellow, white,
	white_on_red_bg, black_on_white_bg, white_on_grey_bg,
	white_on_cyan_bg, default_colour
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

} // log, ember