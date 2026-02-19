/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/utility/ConsoleColour.h>

#ifdef _WIN32
	#include <Windows.h>
#else
	#include <iostream>
	#include <string_view>
#endif

namespace ember::utility {

namespace {

#if defined(_WIN32)

WORD colour_attribute(Colour colour) {
	WORD attribute = 0;
	
	// set foreground
	switch(colour) {
		case Colour::black_on_white_bg:
			[[fallthrough]];
		case Colour::black:
			attribute = 0;
			break;
		case Colour::light_blue:
			attribute = FOREGROUND_INTENSITY;
			[[fallthrough]];
		case Colour::blue:
			attribute |= FOREGROUND_BLUE;
			break;
		case Colour::light_red:
			attribute = FOREGROUND_INTENSITY;
			[[fallthrough]];
		case Colour::red:
			attribute |= FOREGROUND_RED;
			break;
		case Colour::brown:
			attribute = FOREGROUND_GREEN | FOREGROUND_RED;
			break;
		case Colour::light_cyan:
			attribute = FOREGROUND_INTENSITY;
			[[fallthrough]];
		case Colour::cyan:
			attribute |= FOREGROUND_BLUE | FOREGROUND_GREEN;
			break;
		case Colour::light_green:
			attribute = FOREGROUND_INTENSITY;
			[[fallthrough]];
		case Colour::green:
			attribute |= FOREGROUND_GREEN;
			break;
		case Colour::light_magenta:
			attribute = FOREGROUND_INTENSITY;
			[[fallthrough]];
		case Colour::magenta:
			attribute |= FOREGROUND_BLUE | FOREGROUND_RED;
			break;
		case Colour::dark_grey:
			attribute = FOREGROUND_INTENSITY;
			break;
		case Colour::white_on_grey_bg:
			[[fallthrough]];
		case Colour::white_on_red_bg:
			[[fallthrough]];
		case Colour::white_on_cyan_bg:
			[[fallthrough]];
		case Colour::white:
			attribute |= FOREGROUND_INTENSITY;
			[[fallthrough]];
		case Colour::grey:
			attribute |= FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
			break;
		case Colour::yellow:
			attribute = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_RED;
			break;
		case Colour::default_colour:
			// shutting the compiler up
			break;
	}

	// set background
	switch(colour) {
		case Colour::white_on_grey_bg:
			attribute |= BACKGROUND_INTENSITY;
			break;
		case Colour::black_on_white_bg:
			attribute |= BACKGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
			break;
		case Colour::white_on_cyan_bg:
			attribute |= BACKGROUND_GREEN | BACKGROUND_BLUE;
			break;
		case Colour::white_on_red_bg:
			attribute |= BACKGROUND_RED;
			break;
		case Colour::default_colour:
			// shutting the compiler up
			break;
	}

	return attribute;
}

#else

std::string_view ansi_sequence(Colour colour) {
	switch(colour) {
		case Colour::black:
			return "\033[22;30m";
		case Colour::blue:
			return "\033[22;34m";
		case Colour::brown:
			return "\033[22;33m";
		case Colour::cyan:
			return "\033[22;36m";
		case Colour::dark_grey:
			return "\033[01;30m";
		case Colour::green:
			return "\033[22;32m";
		case Colour::grey:
			return "\033[22;37m";
		case Colour::light_blue:
			return "\033[01;34m";
		case Colour::light_cyan:
			return "\033[01;36m";
		case Colour::light_green:
			return "\033[01;32m";
		case Colour::light_magenta:
			return "\033[01;35m";
		case Colour::light_red:
			return "\033[01;31m";
		case Colour::magenta:
			return "\033[22;35m";
		case Colour::red:
			return "\033[22;31m";
		case Colour::white:
			return "\033[01;37m";
		case Colour::yellow:
			return "\033[01;33m";
		case Colour::white_on_red_bg:
			return "\033[01;37m\033[41m";
		case Colour::white_on_grey_bg:
			return "\033[01;37m\033[100m";
		case Colour::white_on_cyan_bg:
			return "\033[01;37m\033[46m";
		case Colour::black_on_white_bg:
			return "\033[01;30m\033[107m";
		default:
			return "\033[0m";
	}
}

#endif

} // unnamed

void set_console_out_colour(Colour colour) {
#if defined(_WIN32)
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(console, colour_attribute(colour));
#else
	std::cout << ansi_sequence(colour);
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

} // util, ember