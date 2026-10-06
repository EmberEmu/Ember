/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/ConsoleColour.h>
#include <logger/detail/PlatformColour.h>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#else
	#include <iostream>
#endif

namespace ember::log {

void set_console_out_colour(Colour colour) {
#if defined(_WIN32)
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(console, detail::colour_attribute(colour));
#else
	std::cout << detail::ansi_sequence(colour);
#endif
}

Colour save_console_out_colour() {
#if defined(_WIN32)
	const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO buffer;
	GetConsoleScreenBufferInfo(stdout_handle, &buffer);
	return static_cast<Colour>(buffer.wAttributes);
#else
	return Colour::default_colour;
#endif
}

} // log, ember