/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once 

#include "HelperMacros.h"
#include "Worker.h"
#include "Sink.h"
#include "Severity.h"
#include "Logger.h"
#include "concurrentqueue.h"
#include "../../threading/Semaphore.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <utility>
#include <tuple>
#include <condition_variable>
#include <thread>

#ifdef _WIN32
	#define thread_local __declspec(thread)
#endif

namespace ember { namespace log {

class Logger::impl {
	friend class Logger;

	Severity severity_ = Severity::DISABLED;
	std::vector<std::unique_ptr<Sink>> sinks_;
	Worker worker_;

	//Workarounds for lack of full thread_local support in VS2013
	std::unordered_map<std::thread::id, std::pair<Severity, std::vector<char>>> buffers_;
	std::unordered_map<std::thread::id, Semaphore<std::mutex>> sync_semaphores_;
	static thread_local std::pair<Severity, std::vector<char>>* t_buffer_;
	static thread_local Semaphore<std::mutex>* t_sem_;
	std::mutex id_lock_;

	void finalise() {
		t_buffer_->second.push_back('\n');
		worker_.queue_.enqueue(std::move(*t_buffer_));
		worker_.signal();
		t_buffer_->second.reserve(64);
	}

	void finalise_sync() {
		t_buffer_->second.push_back('\n');
		auto r = std::make_tuple<Severity, std::vector<char>, Semaphore<std::mutex>*>
					(std::move(t_buffer_->first), std::move(t_buffer_->second), std::move(t_sem_));
		worker_.queue_sync_.enqueue(std::move(r));
		worker_.signal();
		t_buffer_->second.reserve(64);
		t_sem_->wait();
	}

	//Workaround for lack of full thread_local support in VS2013
	void thread_enter() {
		auto id = std::this_thread::get_id();
		std::lock_guard<std::mutex> guard(id_lock_);
		t_buffer_ = &buffers_[id];
		t_sem_ = &sync_semaphores_[id];
	}

public:
	impl() : worker_(sinks_) {
		worker_.start();
	}

	~impl() {
		worker_.stop();
	}

	//Workaround for lack of full thread_local support in VS2013
	void thread_exit() {
		std::lock_guard<std::mutex> guard(id_lock_);
		buffers_.erase(std::this_thread::get_id());
	}

	impl& operator <<(impl& (*m)(impl&)) {
		return (*m)(*this);
	}

	impl& operator <<(Severity severity) {
		//Workaround for lack of full thread_local support in VS2013
		if(t_buffer_ == nullptr) {
			thread_enter();
		}

		t_buffer_->first = severity;
		return *this;
	}

	impl& operator <<(float data) {
		std::string conv = std::to_string(data);
		std::copy(conv.begin(), conv.end(), std::back_inserter(t_buffer_->second));
		return *this;
	}

	impl& operator <<(double data) {
		std::string conv = std::to_string(data);
		std::copy(conv.begin(), conv.end(), std::back_inserter(t_buffer_->second));
		return *this;
	}

	impl& operator <<(bool data) {
		std::string conv = std::to_string(data);
		std::copy(conv.begin(), conv.end(), std::back_inserter(t_buffer_->second));
		return *this;
	}

	impl& operator <<(int data) {
		std::string conv = std::to_string(data);
		std::copy(conv.begin(), conv.end(), std::back_inserter(t_buffer_->second));
		return *this;
	}

	impl& operator <<(unsigned int data) {
		std::string conv = std::to_string(data);
		std::copy(conv.begin(), conv.end(), std::back_inserter(t_buffer_->second));
		return *this;
	}

	impl& operator <<(const std::string& data) {
		std::copy(data.begin(), data.end(), std::back_inserter(t_buffer_->second));
		return *this;
	}

	impl& operator <<(const char* data) {
		std::copy(data, data + std::strlen(data), std::back_inserter(t_buffer_->second));
		return *this;
	}

	Severity severity() {
		return severity_;
	}

	void add_sink(std::unique_ptr<Sink> sink) {
		if(sink->severity() < severity_) {
			severity_ = sink->severity();
		}

		sinks_.emplace_back(std::move(sink));
	}

	impl(const impl&) = delete;
	impl& operator=(const impl&) = delete;
};

std::pair<Severity, std::vector<char>>* Logger::impl::t_buffer_;
Semaphore<std::mutex>* Logger::impl::t_sem_;

}} //log, ember