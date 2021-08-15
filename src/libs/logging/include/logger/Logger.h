/*
 * Copyright (c) 2015  - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once 

#include <logger/Severity.h>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ember::log {

class Sink;

class Logger final {
	class impl;
	std::unique_ptr<impl> pimpl_;

	std::vector<char>* get_buffer();

public:
	Logger();
	~Logger();

	void fmt_write(const Severity severity, const std::string_view fmt) {
		*this << severity << fmt;
		finalise();
	}

	template<typename ... Args>
	void fmt_write(const Severity severity, const std::string_view fmt, Args ... args) {
		*this << severity;
		auto buffer = get_buffer();
		std::format_to(std::back_inserter(*buffer), fmt, args...);
		finalise();
	}

	Logger& operator <<(Logger& (*m)(Logger&));
	Logger& operator <<(Severity severity);
	Logger& operator <<(Filter record_type);
	Logger& operator <<(float data);
	Logger& operator <<(double data);
	Logger& operator <<(bool data);
	Logger& operator <<(int data);
	Logger& operator <<(long data);
	Logger& operator <<(long long data);
	Logger& operator <<(unsigned long data);
	Logger& operator <<(unsigned long long data);
	Logger& operator <<(unsigned int data);
	Logger& operator <<(const std::string& data);
	Logger& operator <<(const std::string_view data);
	Logger& operator <<(const char* data);
	void add_sink(std::unique_ptr<Sink> sink);
	Severity severity();
	Filter filter();
	void finalise();
	void finalise_sync();

	Logger(const impl&) = delete;
	Logger& operator=(const Logger&) = delete;

	friend Logger& flush(Logger& out);
	friend Logger& flush_sync(Logger& out);
};

Logger& flush(Logger& out);
Logger& flush_sync(Logger& out);

} //log, ember