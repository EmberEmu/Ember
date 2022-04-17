/*
 * Copyright (c) 2016 - 2022 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Utility.h"
#include <iostream>

#ifdef _WIN32
    #include <Windows.h>
#endif

namespace ember::util {

std::size_t max_consecutive(std::string_view name) {
	std::size_t current_run = 0;
	std::size_t longest_run = 0;
	char last = 0;

	for(auto c : name) {
		if(c == last) {
			++current_run;
		} else {
			current_run = 1;
		}

		if(current_run > longest_run) {
			longest_run = current_run;
		}

		last = c;
	}

	return longest_run;
}

void set_window_title(std::string_view title) {
#ifdef _WIN32
    SetConsoleTitle(title.data());
#elif defined __linux__ || defined __unix__ // todo, test
  	std::cout << "\033]0;" << title << "}\007";
#endif
}

} // util, ember
