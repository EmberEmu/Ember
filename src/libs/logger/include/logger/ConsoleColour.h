/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Colour.h>

namespace ember::log {

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