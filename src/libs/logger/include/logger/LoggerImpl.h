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
#include <moodycamel/concurrentqueue.h>
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

	static constexpr std::size_t buffer_reserve = 128;

	Severity severity_ = Severity::disabled;
	Filter filter_ = Filter(0);
	std::vector<std::shared_ptr<Sink>> sinks_;
	std::mutex sink_lock_;
	Worker worker_;
	const std::string prefix_;

	static inline thread_local std::tuple<RecordDetail, std::string_view, std::string> buffer_;
	static inline thread_local std::binary_semaphore sem_{0};

	void finalise() {
#ifdef LOG_PRODUCER_BACKPRESSURE
		if(worker_.discard()) {
			buffer_.second.clear();
			return;
		}
#endif
		std::get<std::string_view>(buffer_) = prefix_;
		std::get<std::string>(buffer_).push_back('\n');
		worker_.queue_.enqueue(std::move(buffer_));
#ifndef LOG_PRODUCER_LOW_LATENCY
		worker_.signal();
#endif
		buffer_ = {};
		std::get<std::string>(buffer_).reserve(buffer_reserve);
	}

	void finalise_sync() {
		std::get<std::string>(buffer_).push_back('\n');

		auto r = std::make_tuple<RecordDetail, std::string_view, std::string, std::binary_semaphore*>(
			std::move(std::get<RecordDetail>(buffer_)), prefix_, std::move(std::get<std::string>(buffer_)), &sem_
		);

		worker_.queue_sync_.enqueue(std::move(r));
		worker_.signal();
		buffer_ = {};
		std::get<std::string>(buffer_).reserve(buffer_reserve);
		sem_.acquire();
	}

	std::string& get_buffer() {
		return std::get<std::string>(buffer_);
	}

public:
	impl(std::string prefix) : 
		prefix_(std::move(prefix))
		, worker_(sinks_, sink_lock_) {
		worker_.start();
	}

	impl() : worker_(sinks_, sink_lock_) {
		worker_.start();
	}

	~impl() {
		worker_.stop();
	}

	void copy_to_stream(auto& data) {
		const std::string conv { std::to_string(data) };
		auto& message = std::get<std::string>(buffer_);
		const auto size = message.size();
		message.resize(size + conv.size());
		std::ranges::copy(conv, message.data() + size);
	}

	impl& operator <<(impl& (*m)(impl&)) {
		std::get<RecordDetail>(buffer_).type = 1; // first bit represents the misc. record type
		return (*m)(*this);
	}

	impl& operator <<(Severity severity) {
		std::get<RecordDetail>(buffer_).type = 1; // first bit represents the misc. record type
		std::get<RecordDetail>(buffer_).severity = severity;
		return *this;
	}

	impl& operator <<(Filter type) {
		std::get<RecordDetail>(buffer_).type = type;
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
		auto& message = std::get<std::string>(buffer_);
		const auto size = message.size();
		message.resize(size + data.size());
		std::ranges::copy(data, message.data() + size);
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

		for(const auto& s : sinks_) {
			if(sink->unique()) {
				if(s->type() == sink->type()) {
					throw std::runtime_error(
						std::format(
							R"(Attempt to register multiple instances of a sink, "{}", declared as unique)", sink->type()
						)
					);
				}
			}

			if(s->name() == sink->name()) {
				throw std::runtime_error(
					std::format(
						R"(Attempt to register sink with non-unique name, "{}")", sink->name()
					)
				);
			}
		}

		if(sink->severity() < severity_) {
			severity_ = sink->severity();
		}

		filter_ |= sink->filter();
		sinks_.emplace_back(std::move(sink));
	}

	bool remove_sink(const std::shared_ptr<Sink>& remove) {
		std::lock_guard lock(sink_lock_);

		if(auto it = std::ranges::find(sinks_, remove); it != sinks_.end()) {
			sinks_.erase(it);
			return true;
		}

		return false;
	}

	bool remove_sink(const std::string_view name) {
		std::lock_guard lock(sink_lock_);
		std::shared_ptr<Sink> target;

		for(auto& sink : sinks_) {
			if(sink->name() == name) {
				target = sink;
			}
		}

		if(!target) {
			return false;
		}

		return remove_sink(target);
	}

	std::vector<std::shared_ptr<Sink>> fetch_sinks() {
		std::lock_guard lock(sink_lock_);
		return sinks_;
	}

	std::vector<std::shared_ptr<Sink>> fetch_sinks(const std::string_view type) {
		std::lock_guard lock(sink_lock_);
		std::vector<std::shared_ptr<Sink>> sinks;

		for(auto& sink : sinks_) {
			if(sink->type() == type) {
				sinks.emplace_back(sink);
			}
		}

		return sinks;
	}

	std::shared_ptr<Sink> fetch_sink(const std::string_view name) {
		std::lock_guard lock(sink_lock_);
		std::vector<std::shared_ptr<Sink>> sinks;

		for(auto& sink : sinks_) {
			if(sink->name() == name) {
				return sink;
			}
		}

		return nullptr;
	}

	std::shared_ptr<Sink> fetch_unique(const std::string_view type) {
		std::lock_guard lock(sink_lock_);
		std::shared_ptr<Sink> sink;

		for(auto& sink : sinks_) {
			if(sink->type() == type) {
				if(!sink->unique()) {
					throw std::runtime_error("attempted to fetch non-unique sink type from fetch_unique");
				}

				return sink;
			}
		}

		return nullptr;
	}

	void set_soft_limit(std::size_t value) {
		worker_.set_soft_limit(value);
	}

	void set_hard_limit(std::size_t value) {
		worker_.set_hard_limit(value);
	}

	impl(const impl&) = delete;
	impl& operator=(const impl&) = delete;
	impl(const impl&&) = delete;
	impl& operator=(const impl&&) = delete;
};

} // log, ember