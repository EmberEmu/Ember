/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Sink.h>
#include <moodycamel/concurrentqueue.h>
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
	moodycamel::ConsumerToken queue_tok_;
	moodycamel::ConsumerToken squeue_tok_;
	std::vector<std::pair<RecordDetail, std::vector<char>>> dequeued_;
	std::vector<std::shared_ptr<Sink>>& sinks_;
	std::mutex& sink_lock_;
	std::binary_semaphore sem_;
	std::thread thread_;
	std::atomic_bool stop_ { false };
#ifdef LOG_PRODUCER_BACKPRESSURE
	std::atomic_bool discard_ { false };
#endif
	std::size_t soft_queue_limit = 1000;
	std::size_t hard_queue_limit = 2000;
	unsigned int discarded_ = 0;
	
	void apply_hard_backpressure(std::size_t size);
	void apply_soft_backpressure(std::size_t size);
	void write_discard_log();
	void process_dequeued();
	void process_outstanding();
	void process_outstanding_sync();
	void run();

	friend class Logger;

public:
	Worker(std::vector<std::shared_ptr<Sink>>& sinks, std::mutex& sink_lock);
	~Worker();

	void start();
	void stop();
	bool discard();
	void set_soft_limit(std::size_t value);
	void set_hard_limit(std::size_t value);

	inline void signal() { 
		sem_.release();
	}
};

} // log, ember