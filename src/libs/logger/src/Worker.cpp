/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/Worker.h>
#include <thread/Utility.h>
#include <chrono>
#include <iterator>
#include <format>

using namespace std::chrono_literals;

namespace ember::log {

Worker::~Worker() {
	if(!stop_) {
		stop();
	}
}

bool Worker::discard() {
#ifdef LOG_PRODUCER_BACKPRESSURE
	const auto val = discard_.load(std::memory_order_relaxed);

	if(val) {
		++discarded_;
	}

	return val;
#else
	return false;
#endif
}

void Worker::apply_hard_backpressure(const std::size_t size_approx) {
	if(hard_queue_limit == 0 || size_approx < hard_queue_limit) {
#ifdef LOG_PRODUCER_BACKPRESSURE
		discard_.store(false, std::memory_order_relaxed);
#endif
		return;
	}

	std::pair<RecordDetail, std::vector<char>> item;
	const auto discard_count = size_approx - hard_queue_limit;

	for(auto i = 0u; i < discard_count; ++i) {
		if(queue_.try_dequeue(item)) {
			++discarded_;
		}
	}

#ifdef LOG_PRODUCER_BACKPRESSURE
	discard_.store(true, std::memory_order_relaxed);
#endif
}

void Worker::apply_soft_backpressure(const std::size_t size) {
	if(soft_queue_limit == 0 || size < soft_queue_limit) {
		return;
	}

	const auto discard_count = size - soft_queue_limit;

	for(auto i = 0u; i < discard_count; ++i) {
		auto& [detail, _] = dequeued_.back();

		if(detail.severity == Severity::trace || detail.severity == Severity::debug) {
			dequeued_.pop_back();
			++discarded_;
		}
	}
}

void Worker::write_discard_log() {
	std::lock_guard lock(sink_lock_);

	const auto msg = std::format("Logger discarded {} entries due to pressure", discarded_);

	for(auto& s : sinks_) {
		s->write(Severity::warn, Filter{0}, msg, true);
	}
}

void Worker::process_outstanding_sync() {
	std::tuple<RecordDetail, std::vector<char>, std::binary_semaphore*> item;

	std::lock_guard lock(sink_lock_);

	while(queue_sync_.try_dequeue(item)) {
		for(auto& s : sinks_) {
			s->write(std::get<0>(item).severity, std::get<0>(item).type, std::get<1>(item), true);
		}

		std::get<2>(item)->release();
	}
}

void Worker::process_dequeued() {
	const auto records = dequeued_.size();
	apply_soft_backpressure(records);

	if(discarded_) {
		write_discard_log();
		discarded_ = 0;
	}

	std::lock_guard lock(sink_lock_);

	if(records < batch_write_threshold) {
		for(auto& s : sinks_) {
			for(auto& [detail, data] : dequeued_) {
				s->write(detail.severity, detail.type, data, false);
			}
		}
	} else {
		for(auto& s : sinks_) {
			s->batch_write(dequeued_);
		}
	}

	dequeued_.clear();

	if(dequeued_.capacity() > max_dequed_capacity && records < max_dequed_capacity) {
		dequeued_.shrink_to_fit();
	}
}

void Worker::process_outstanding() {
	const auto size_approx = queue_.size_approx();
	apply_hard_backpressure(size_approx);
	dequeued_.reserve(size_approx);

	if(queue_.try_dequeue_bulk(std::back_inserter(dequeued_), hard_queue_limit)) {
		process_dequeued();
	}
}

void Worker::set_soft_limit(const std::size_t value) {
	soft_queue_limit = value;
}

void Worker::set_hard_limit(const std::size_t value) {
	hard_queue_limit = value;
}

void Worker::run() {
	while(!stop_) {
#ifdef LOG_PRODUCER_LOW_LATENCY
		const bool ret = sem_.try_acquire_for(100ms);
#else
		const bool ret = sem_.try_acquire_for(250ms);
#endif
		process_outstanding();

		if(ret) {
			process_outstanding_sync();
		}
	}
}

void Worker::start() {
	thread_ = std::thread(&Worker::run, this);
	thread::set_name(thread_, "Log Worker");
}

void Worker::stop() {
	if(thread_.joinable()) {
		stop_ = true;
		sem_.release();
		thread_.join();
		process_outstanding();
		process_outstanding_sync();
	}
}

} // log, ember