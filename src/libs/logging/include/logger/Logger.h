/*
 * Copyright (c) 2015  - 2024 Ember
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
#include <utility>
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

	template<bool async, typename ... Args>
	constexpr void fmt_write(const Severity severity, std::format_string<Args...> fmt, Args&&... args) {
		*this << severity;
		auto buffer = get_buffer();

		std::format_to(std::back_inserter(*buffer),
		               std::forward<std::format_string<Args...>>(fmt),
		               std::forward<Args>(args)...);

		if constexpr (async) {
			finalise();
		} else {
			finalise_sync();
		}
	}

	Logger& operator <<(Logger& (*m)(Logger&));
	Logger& operator <<(Severity severity);
	Logger& operator <<(Filter record_type);
	Logger& operator <<(std::string_view data);
	Logger& operator <<(float data);
	Logger& operator <<(double data);
	Logger& operator <<(bool data);
	Logger& operator <<(const char* data);


	template<unsigned int N>
	Logger& operator <<(const char(&data)[N]) {
		return *this << std::string_view(data, N-1);
	}

	Logger& operator <<(int data);
	Logger& operator <<(long data);
	Logger& operator <<(long long data);
	Logger& operator <<(unsigned long data);
	Logger& operator <<(unsigned long long data);
	Logger& operator <<(unsigned int data);
	void add_sink(std::unique_ptr<Sink> sink);
	Severity severity();
	Filter filter();
	void finalise();
	void finalise_sync();

	Logger* operator->() {
		return this;
	}

	Logger(Logger&) = delete;
	Logger& operator=(const Logger&) = delete;
	Logger(Logger&&) = delete;
	Logger& operator=(Logger&&) = delete;

	friend Logger& flush(Logger& out);
	friend Logger& flush_sync(Logger& out);
};

Logger& flush(Logger& out);
Logger& flush_sync(Logger& out);

} // log, ember