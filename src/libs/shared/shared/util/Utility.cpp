/*
 * Copyright (c) 2016 - 2019 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Utility.h"

#ifdef _WIN32
    #include <Windows.h>
#endif

namespace ember::util {

// todo, not unicode aware
std::size_t max_consecutive_check(std::string_view name) {
	int longest_sequence = 1;
	int current_sequence = 1;
	char last_letter = 0;
	bool reset = false;

	for(auto c : name) {
		if(c == last_letter) {
			++current_sequence;
			reset = false;
		} else {
			reset = true;
		}

		if(current_sequence > longest_sequence) {
			longest_sequence = current_sequence;
		}

		if(reset) {
			current_sequence = 1;
		}

		last_letter = c;
	}

	return longest_sequence;
}

void set_window_title(std::string_view title) {
#ifdef _WIN32
    SetConsoleTitle(title.data());
#elif defined __linux__ || defined __unix__ // todo, test
  	std::cout << "\033]0;" << title << "}\007";
#endif
}

} // util, ember
