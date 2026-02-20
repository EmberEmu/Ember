/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/HelperMacros.h>
#include <logger/Worker.h>
#include <logger/Sink.h>
#include <logger/Severity.h>
#include <logger/Logger.h>
#include <logger/concurrentqueue.h>
#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <semaphore>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>
#include <cstddef>
#include <cstring>

namespace ember::log {

class Logger::impl final {
	friend class Logger;

	static constexpr std::size_t BUFFER_RESERVE = 256;

	Severity severity_ = Severity::disabled;
	Filter filter_ = Filter(0);
	std::vector<std::shared_ptr<Sink>> sinks_;
	std::mutex sink_lock_;
	Worker worker_;

	static inline thread_local std::pair<RecordDetail, std::vector<char>> buffer_;
	static inline thread_local std::binary_semaphore sem_{0};

	void finalise() {
		buffer_.second.push_back('\n');
		worker_.queue_.enqueue(std::move(buffer_));
		worker_.signal();
		buffer_ = {};
		buffer_.second.reserve(BUFFER_RESERVE);
	}

	void finalise_sync() {
		buffer_.second.push_back('\n');
		auto r = std::make_tuple<RecordDetail, std::vector<char>, std::binary_semaphore*>
					(std::move(buffer_.first), std::move(buffer_.second), &sem_);
		worker_.queue_sync_.enqueue(std::move(r));
		worker_.signal();
		buffer_ = {};
		buffer_.second.reserve(BUFFER_RESERVE);
		sem_.acquire();
	}

	std::vector<char>* get_buffer() {
		return &buffer_.second;
	}

public:
	impl() : worker_(sinks_, sink_lock_) {
#ifndef DEBUG_NO_THREADS
		worker_.start();
#endif
	}

	~impl() {
#ifndef DEBUG_NO_THREADS
		worker_.stop();
#endif
	}

	void copy_to_stream(auto& data) {
		const std::string conv { std::to_string(data) };
		const auto size = buffer_.second.size();
		buffer_.second.resize(size + conv.size());
		std::ranges::copy(conv, buffer_.second.data() + size);
	}

	impl& operator <<(impl& (*m)(impl&)) {
		buffer_.first.type = 1; // first bit represents the misc. record type
		return (*m)(*this);
	}

	impl& operator <<(Severity severity) {
		buffer_.first.type = 1; // first bit represents the misc. record type
		buffer_.first.severity = severity;
		return *this;
	}

	impl& operator <<(Filter type) {
		buffer_.first.type = type;
		return *this;
	}

	impl& operator <<(bool data) {
		copy_to_stream(data);
		return *this;
	}

	impl& operator <<(float data) {
		copy_to_stream(data);
		return *this;
	}

	impl& operator <<(double data) {
		copy_to_stream(data);
		return *this;
	}

	impl& operator <<(int data) {
		copy_to_stream(data);
		return *this;
	}

	impl& operator <<(unsigned int data) {
		copy_to_stream(data);
		return *this;
	}

	impl& operator <<(long data) {
		copy_to_stream(data);
		return *this;
	}

	impl& operator <<(unsigned long data) {
		copy_to_stream(data);
		return *this;
	}

	impl& operator <<(unsigned long long data) {
		copy_to_stream(data);
		return *this;
	}

	impl& operator <<(long long data) {
		copy_to_stream(data);
		return *this;
	}

	impl& operator <<(const std::string_view data) {
		const auto size = buffer_.second.size();
		buffer_.second.resize(size + data.size());
		std::ranges::copy(data, buffer_.second.data() + size);
		return *this;
	}

	Severity severity() const {
		return severity_;
	}

	Filter filter() const {
		return filter_;
	}

	void add_sink(std::shared_ptr<Sink> sink) {
		std::lock_guard lock(sink_lock_);

		if(sink->unique()) {
			for(auto& s : sinks_) {
				if(s->name() == sink->name()) {
					throw std::runtime_error(
						std::format(R"(Attempt to register multiple instances of a sink, "{}", declared as unique)", sink->name())
					);
				}
			}
		}

		if(sink->severity() < severity_) {
			severity_ = sink->severity();
		}

		filter_ |= sink->filter();
		sinks_.emplace_back(std::move(sink));
	}

	std::vector<std::shared_ptr<Sink>> fetch_sinks() {
		std::lock_guard lock(sink_lock_);
		return sinks_;
	}

	std::vector<std::shared_ptr<Sink>> fetch_sink(const std::string_view name) {
		std::lock_guard lock(sink_lock_);
		std::vector<std::shared_ptr<Sink>> sinks;

		for(auto& sink : sinks_) {
			if(sink->name() == name) {
				sinks.emplace_back(sink);
			}
		}

		return sinks;
	}

	impl(const impl&) = delete;
	impl& operator=(const impl&) = delete;
	impl(const impl&&) = delete;
	impl& operator=(const impl&&) = delete;
};

} // log, ember