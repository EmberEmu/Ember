/*
 * Copyright (c) 2015 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/Utility.h>
#include <logger/Exception.h>
#include <chrono>
#include <iomanip>
#include <cstring>

namespace ember::log {

namespace detail {

namespace sc = std::chrono;

std::tm current_time() {
	std::tm time;
	auto sys_time = sc::system_clock::to_time_t(sc::system_clock::now());

#if _MSC_VER && !__INTEL_COMPILER
	localtime_s(&time, &sys_time);
#else
	localtime_r(&sys_time, &time);
#endif

	return time;
}

std::size_t put_time(const std::tm& time, cstring_view format, std::span<char> buffer) {
	const auto res = std::strftime(buffer.data(), buffer.size(), format.c_str(), &time);

	if(!res) [[unlikely]] {
		const char err[] = "[error]";

		if(buffer.size() >= sizeof(err)) {
			std::memcpy(std::data(buffer), err, sizeof(err));
		} else {
			buffer[0] = '\0';
		}
	}

	return res;
}

std::string put_time(const std::tm& time, cstring_view format) {
	const std::size_t BUFFER_SIZE = 128;
	char buffer[BUFFER_SIZE];

	if(!std::strftime(buffer, BUFFER_SIZE, format.c_str(), &time)) {
		return "[error]";
	}

	return buffer;
}

std::string_view severity_string(Severity severity) {
	switch(severity) {
		case Severity::debug:
			return "[debug] ";
		case Severity::trace:
			return "[trace] ";
		case Severity::warn:
			return "[warning] ";
		case Severity::info:
			return "[info] ";
		case Severity::ERROR_:
			return "[error] ";
		case Severity::fatal:
			return "[fatal] ";
		case Severity::console_error:
			[[fallthrough]];
		case Severity::console:
			return "[console] ";
		default:
			return "[unknown] ";
	}
}

} // detail

Severity severity_string(std::string_view severity) {
	if(severity == "trace") {
		return Severity::trace;
	} else if(severity == "debug") {
		return Severity::debug;
	} else if(severity == "info") {
		return Severity::info;
	} else if(severity == "warning") {
		return Severity::warn;
	} else if(severity == "error") {
		return Severity::ERROR_;
	} else if(severity == "fatal") {
		return Severity::fatal;
	} else if(severity == "none" || severity == "disabled") {
		return Severity::disabled;
	} else {
		throw exception("Unknown severity passed to severity_string");
	}
}

} // log, ember