/*
 * Copyright (c) 2015 - 2022 Ember
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
#include <vector>
#include <mutex>
#include <thread>
#include <memory>
#include <semaphore>
#include <string>
#include <tuple>
#include <condition_variable>

namespace ember::log {

class Worker final {
	moodycamel::ConcurrentQueue<std::pair<RecordDetail, std::vector<char>>> queue_;
	moodycamel::ConcurrentQueue<std::tuple<RecordDetail, std::vector<char>, std::binary_semaphore*>> queue_sync_;
	std::vector<std::pair<RecordDetail, std::vector<char>>> dequeued_;
	std::vector<std::unique_ptr<Sink>>& sinks_;
	std::binary_semaphore sem_;
	std::thread thread_;
	std::atomic_bool stop_ { false };

	void process_outstanding();
	void process_outstanding_sync();
	void run();

	friend class Logger;

public:
	Worker(std::vector<std::unique_ptr<Sink>>& sinks) : sinks_(sinks), sem_(0) {}
	~Worker();

	void start();
	void stop();
	inline void signal() { 
		sem_.release();
#ifdef DEBUG_NO_THREADS
		run();
#endif
	}
};

} //log, ember