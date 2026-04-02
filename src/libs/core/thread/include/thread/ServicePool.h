/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <thread>
#include <vector>
#include <cstddef>

namespace ember::thread {

class ServicePool final {
	static constexpr auto asio_concurrency_hint = 1;

	const int hint_;
	std::size_t pool_size_;
	std::size_t next_service_;
	std::vector<std::unique_ptr<boost::asio::io_context>> services_;
	std::vector<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_;
	std::vector<std::jthread> threads_;

	std::size_t limit_pool_size(const std::size_t pool_size);
	void initialise_pool();
	
public:
	explicit ServicePool(std::size_t pool_size, int hint = asio_concurrency_hint);
	~ServicePool();

	/*
	 * Begins running the pool. This function is non-blocking and cannot be called again
	 * without first calling 'stop' or 'shutdown'.
	 */
	void run(bool pin = true, unsigned int concurrency = std::thread::hardware_concurrency());

	/*
	 * Requests a worker from the pool. This selects a worker using round-robin. 
	 */
	boost::asio::io_context& get_next();

	/*
	 * Requests a specific worker from the pool. 
	 * 
	 * If the requested index is invalid, an exception will be thrown.
	 */
	boost::asio::io_context& get(std::size_t index) const;

	/*
	 * Requests a specific worker from the pool. 
	 * 
	 * If the requested index is invalid, a nullptr will be returned.
	 */
	boost::asio::io_context* get_if(std::size_t index) const;

	/*
	 * Requests that the pool stops running immediately. Any outstanding work will
	 * be cancelled.
	 */
	void stop();

	/*
	 * Begins pool shutdown but waits until all work has been completed. This should
	 * be used for a graceful shutdown where all components using the pool have 
	 * also been told to stop/shutdown. If this is not the case, it could block indefinitely.
	 */
	void shutdown();

	/*
	 * Returns the number of workers and threads in this pool.
	 */
	std::size_t size() const;

	auto begin() const {
		return services_.begin();
	}

	auto end() const {
		return services_.end();
	}

	auto begin() {
		return services_.begin();
	}

	auto end() {
		return services_.end();
	}

	ServicePool(const ServicePool&) = delete;
	ServicePool& operator=(const ServicePool&) = delete;
};

} // thread, ember