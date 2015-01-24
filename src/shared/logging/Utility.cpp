/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Utility.h>
#include <logger/Exception.h>
#include <time.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ember { namespace log {

SEVERITY severity_string(const std::string& severity) {
	if(severity == "trace") {
		return SEVERITY::TRACE;
	} else if(severity == "debug") {
		return SEVERITY::DEBUG;
	} else if(severity == "info") {
		return SEVERITY::INFO;
	} else if(severity == "warning") {
		return SEVERITY::WARN;
	} else if(severity == "error") {
		return SEVERITY::ERROR;
	} else if(severity == "fatal") {
		return SEVERITY::FATAL;
	} else {
		throw exception("Unknown severity passed to severity_string");
	}
}

namespace detail {

namespace sc = std::chrono;

std::tm current_time() {
	std::tm time;
	auto sys_time = sc::system_clock::to_time_t(sc::system_clock::now());

#ifdef _MSC_VER
	localtime_s(&time, &sys_time);
#else
	localtime_r(&sys_time, &time);
#endif

	return time;
}

std::string put_time(const std::tm& time, const std::string& format) {
#if defined __GNUC__  || defined __MINGW32__
	const std::size_t BUFFER_SIZE = 128;
	char buffer[BUFFER_SIZE];

	if(!std::strftime(buffer, BUFFER_SIZE, format.c_str(), &time)) {
		return "[error]";
	}
#else
	auto out = std::put_time(&time, format.c_str());
	std::stringstream stream;
	stream << out;
	const std::string& buffer = stream.str();
#endif
	return buffer;
}

std::string severity_string(SEVERITY severity) {
	switch(severity) {
		case SEVERITY::DEBUG:
			return "[debug] ";
		case SEVERITY::TRACE:
			return "[trace] ";
		case SEVERITY::WARN:
			return "[warning] ";
		case SEVERITY::INFO:
			return "[info] ";
		case SEVERITY::FATAL:
			return "[fatal] ";
		case SEVERITY::ERROR:
			return "[error] ";
		default:
			return "[unknown] ";
	}
}

}}} //detail