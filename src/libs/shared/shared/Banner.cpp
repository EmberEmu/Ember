/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Banner.h"
#include "Version.h"
#include "rlutil.h"
#include <iostream>

namespace ember {

void print_banner(const std::string& display_name) {
	rlutil::saveColor();

	rlutil::setColor(rlutil::DARKGREY);
	std::cout << "\n"
		R"(                                      d8b)" << "\n";
	rlutil::setColor(rlutil::GREY);
	std::cout <<
		R"(                                      ?88)" << "\n";
	rlutil::setColor(rlutil::YELLOW);
	std::cout <<
		R"(                                       88b)" << "\n"
		R"(       )         d8888b  88bd8b,d88b   888888b  d8888b  88bd88b)" << "\n";
	rlutil::setColor(rlutil::LIGHTRED);
	std::cout <<
		R"(      ) \       d8b_,dP  88P'`?8P'?8b  88P `?8bd8b_,dP  88P'  `)" << "\n";
	rlutil::setColor(rlutil::RED);
	std::cout <<
		R"(     / ) (      88b     d88  d88  88P d88,  d8888b     d88)" << "\n"
		R"(     \(_)/      `?888P'd88' d88'  88bd88'`?88P'`?888P'd88')" << "\n\n";

	rlutil::resetColor();
	std::cout << display_name << ", v" << version::VERSION << " (" << version::GIT_HASH << ")\n\n";
}

}