/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Utility.h"

#ifdef _WIN32
    #include <Windows.h>
#elif defined __linux__ || defined __unix__ || defined __APPLE__
	#include <sys/resource.h>
	#include <iostream>
#endif

namespace ember::util {

std::size_t max_consecutive(std::string_view name, const bool case_insensitive, const std::locale& locale) {
	std::size_t current_run = 0;
	std::size_t longest_run = 0;
	char last = 0;

	for(auto c : name) {
		if(case_insensitive) {
			c = std::tolower(c, locale);
		}

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

void set_window_title(cstring_view title) {
#ifdef _WIN32
    SetConsoleTitle(title.c_str());
#elif defined __linux__ || defined __unix__ // todo, test
  	std::cout << "\033]0;" << title << "}\007";
#endif
}

int max_sockets() {
#if defined __linux__ || defined __unix__ || defined __APPLE__
	rlimit limit{};
	const int res = getrlimit(RLIMIT_NOFILE, &limit);
	
	if(res == 0) {
		return -1;
	}

	return limit.rlim_cur;
#endif
	return 0;
}

std::string max_sockets_desc() {
	int max = max_sockets();
	std::string value;

	if(max == -1) {
		value = "unable to retrieve value";
	} else if(max == 0) {
		value = "no known limits";
	} else {
		value = std::to_string(max);
	}

	return value;
}

} // util, ember
