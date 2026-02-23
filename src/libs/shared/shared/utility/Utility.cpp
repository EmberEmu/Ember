/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Utility.h"
#include <shared/CompilerWarn.h>
#include <botan/bigint.h>
#include <format>
#include <cassert>

using namespace std::chrono_literals;

#if defined __APPLE__
	#include <TargetConditionals.h>
#endif

#ifdef _WIN32
    #include <Windows.h>
#elif defined __linux__ || defined __unix__ || defined TARGET_OS_MAC
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

// page locking stuff
#if defined _WIN32
	#include <memoryapi.h>
#elif defined __linux__ || defined __unix__  || defined TARGET_OS_MAC
	#include <sys/mman.h>
#endif

namespace ember::utility {

std::size_t max_consecutive(const std::string_view name, const bool case_insensitive,
                            const std::locale& locale) {
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
#if defined __linux__ || defined __unix__ || defined TARGET_OS_MAC
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

#define STRINGIZE_CASE(x) \
case x: \
	return #x;

std::string sig_str(const int signal) {
#if defined _WIN32 || defined TARGET_OS_MAC
	switch(signal) {
		STRINGIZE_CASE(SIGINT)
		STRINGIZE_CASE(SIGILL)
		STRINGIZE_CASE(SIGFPE)
		STRINGIZE_CASE(SIGSEGV)
		STRINGIZE_CASE(SIGTERM)
#endif
#ifdef  _WIN32
		STRINGIZE_CASE(SIGBREAK)
		STRINGIZE_CASE(SIGABRT)
#elif defined TARGET_OS_MAC
		STRINGIZE_CASE(SIGHUP)
		STRINGIZE_CASE(SIGQUIT)
		STRINGIZE_CASE(SIGTRAP)
		STRINGIZE_CASE(SIGABRT)
		STRINGIZE_CASE(SIGEMT)
		STRINGIZE_CASE(SIGKILL)
		STRINGIZE_CASE(SIGBUS)
		STRINGIZE_CASE(SIGSYS)
		STRINGIZE_CASE(SIGPIPE)
		STRINGIZE_CASE(SIGALRM)
		STRINGIZE_CASE(SIGURG)
		STRINGIZE_CASE(SIGSTOP)
		STRINGIZE_CASE(SIGTSTP)
		STRINGIZE_CASE(SIGCONT)
		STRINGIZE_CASE(SIGCHLD)
		STRINGIZE_CASE(SIGTTIN)
		STRINGIZE_CASE(SIGTTOU)
		STRINGIZE_CASE(SIGIO)
		STRINGIZE_CASE(SIGXCPU)
		STRINGIZE_CASE(SIGXFSZ)
		STRINGIZE_CASE(SIGVTALRM)
		STRINGIZE_CASE(SIGPROF)
		STRINGIZE_CASE(SIGWINCH)
		STRINGIZE_CASE(SIGINFO)
		STRINGIZE_CASE(SIGUSR1)
		STRINGIZE_CASE(SIGUSR2)
#endif
#if defined _WIN32 || defined TARGET_OS_MAC
		default:
			return "<unknown>";
	}
#elif defined __linux__ || defined __unix__
	auto res = sigabbrev_np(signal);

	if(res) {
		return std::format("SIG{}", res);
	} else {
		return "<unknown>";
	}
#else
	#pragma message WARN("Implement sig_str for this platform, thanks");
	return "<unknown>";
#endif
}

bool page_lock(void* address, std::size_t length) {
#if defined _WIN32
	return VirtualLock(address, length) != 0;
#elif defined __linux__ || defined __unix__|| defined TARGET_OS_MAC
	return mlock(address, length) == 0;
#else
	#pragma message WARN("Implement page_lock for this platform, thanks");
	return false;
#endif
}

bool page_unlock(void* address, std::size_t length) {
#if defined _WIN32
	return VirtualUnlock(address, length) != 0;
#elif defined __linux__ || defined __unix__|| defined TARGET_OS_MAC
	return munlock(address, length) == 0;
#else
	#pragma message WARN("Implement page_unlock for this platform, thanks");
	return false;
#endif
}

std::uint32_t to_u32bit(const Botan::BigInt& value) {
	if(value.is_negative()) {
		throw Botan::Encoding_Error("BigInt::to_u32bit: Number is negative");
	}

	if(value.bits() > 32) {
		throw Botan::Encoding_Error("BigInt::to_u32bit: Number is too big to convert");
	}

	uint32_t out = 0;

	for(size_t i = 0; i != 4; ++i) {
		out = (out << 8) | value.byte_at(3 - i);
	}

	return out;
}

std::string time_duration_format(std::chrono::nanoseconds uptime) {
	auto uptime_s = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();

	const auto days = uptime_s / (60 * 60 * 24);
	uptime_s %= (60 * 60 * 24);
	const auto hours = uptime_s / (60 * 60);
	uptime_s %= (60 * 60);
	const auto minutes = uptime_s / 60;
	const auto seconds = uptime_s % 60;
	
	return std::format("{} day{}, {} hour{}, {} minute{}, {} second{}",
		days, (days != 1)? "s" : "", hours, (hours != 1)? "s" : "",
		minutes, (minutes != 1)? "s" : "", seconds, (seconds != 1)? "s" : "");
}

std::string start_time_format(std::chrono::steady_clock::time_point start_time) {
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start_time
	);

	if(elapsed < 1s) {
		return std::format("{}", elapsed);
	} else {
		return std::format("{:.3f}s", elapsed.count() / 1000.0);
	}
}

} // utility, ember
