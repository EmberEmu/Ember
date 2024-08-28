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

// signal stuff
#if defined _WIN32 || defined TARGET_OS_MAC
	#include <signal.h>
#elif defined __linux__ || defined __unix__
	#include <format>
	#include <string.h>
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

std::string sig_str(const int signal) {
#if defined _WIN32 || defined TARGET_OS_MAC
	switch(signal) {
#endif
#ifdef  _WIN32
		case SIGINT:
			return "SIGINT";
		case SIGILL:
			return "SIGILL";
		case SIGFPE:
			return "SIGFPE";
		case SIGSEGV:
			return "SIGSEGV";
		case SIGTERM:
			return "SIGTERM";
		case SIGBREAK:
			return "SIGBREAK";
		case SIGABRT:
			return "SIGABORT";
#elif defined TARGET_OS_MAC
		case SIGHUP:
			return "SIGHUP";
		case SIGINT:
			return "SIGINT";
		case SIGQUIT:
			return "SIGQUIT";
		case SIGILL:
			return "SIGILL";
		case SIGTRAP:
			return "SIGTRAP";
		case SIGABRT:
			return "SIGABRT";
		case SIGEMT:
			return "SIGEMT";
		case SIGFPE:
			return "SIGFPE";
		case SIGKILL:
			return "SIGKILL";
		case SIGBUS:
			return "SIGBUS";
		case SIGSEGV:
			return "SIGSEGV";
		case SIGSYS:
			return "SIGSYS";
		case SIGPIPE:
			return "SIGPIPE";
		case SIGALRM:
			return "SIGALRM";
		case SIGTERM:
			return "SIGTERM";
		case SIGURG:
			return "SIGURG";
		case SIGSTOP:
			return "SIGSTOP";
		case SIGTSTP:
			return "SIGTSTP";
		case SIGCONT:
			return "SIGCONT";
		case SIGCHLD:
			return "SIGCHLD";
		case SIGTTIN:
			return "SIGTTIN";
		case SIGTTOU:
			return "SIGTTOU";
		case SIGIO:
			return "SIGIO";
		case SIGXCPU:
			return "SIGXCPU";
		case SIGXFSZ:
			return "SIGXFSZ";
		case SIGVTALRM:
			return "SIGVTALRM";
		case SIGPROF:
			return "SIGPROF";
		case SIGWINCH:
			return "SIGWINCH";
		case SIGINFO:
			return "SIGINFO";
		case SIGUSR1:
			return "SIGUSR1";
		case SIGUSR2:
			return "SIGUSR2";
#endif
#if defined _WIN32 || defined TARGET_OS_MAC
		default:
			return "UNKNOWN";
	}
#elif defined __linux__ || defined __unix__
	auto res = sigabbrev_np(signal);

	if(res) {
		return std::format("SIG{}", res);
	} else {
		return "unknown";
	}
#else
	static_assert(false, "Implement sig_str for this platform, thanks");
#endif
}

} // util, ember
