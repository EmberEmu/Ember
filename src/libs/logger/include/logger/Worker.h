/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Sink.h>
#include <logger/concurrentqueue.h>
#include <logger/Logger.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <semaphore>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace ember::log {

class Worker final {
	static const std::size_t batch_write_threshold = 10;
	static const std::size_t max_dequed_capacity   = 100;

	moodycamel::ConcurrentQueue<std::pair<RecordDetail, std::vector<char>>> queue_;
	moodycamel::ConcurrentQueue<std::tuple<RecordDetail, std::vector<char>, std::binary_semaphore*>> queue_sync_;
	std::vector<std::pair<RecordDetail, std::vector<char>>> dequeued_;
	std::vector<std::shared_ptr<Sink>>& sinks_;
	std::mutex& sink_lock_;
	std::binary_semaphore sem_;
	std::thread thread_;
	std::atomic_bool stop_ { false };

	void process_outstanding();
	void process_outstanding_sync();
	void run();

	friend class Logger;

public:
	Worker(std::vector<std::shared_ptr<Sink>>& sinks, std::mutex& sink_lock)
		: sinks_(sinks),
		  sink_lock_(sink_lock),
		  sem_(0) {}
	~Worker();

	void start();
	void stop();
	inline void signal() { 
		sem_.release();
	}
};

} // log, ember