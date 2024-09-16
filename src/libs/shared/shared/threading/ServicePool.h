/*
 * Copyright (c) 2016 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/container/small_vector.hpp>
#include <memory>
#include <thread>
#include <vector>
#include <cstddef>

namespace ember {

class ServicePool final {
	static constexpr auto POOL_SIZE_HINT = 16;
	static constexpr auto ASIO_CONCURRENCY_HINT = 1;

	std::size_t pool_size_;
	std::size_t next_service_;
	boost::container::small_vector<std::unique_ptr<boost::asio::io_context>, POOL_SIZE_HINT> services_;
	std::vector<std::shared_ptr<boost::asio::io_context::work>> work_;
	std::vector<std::jthread> threads_;

public:
	explicit ServicePool(std::size_t pool_size);
	~ServicePool();

	boost::asio::io_context& get_service();
	boost::asio::io_context* get_service(std::size_t index) const;
	void run();
	void stop();
	std::size_t size() const;

	ServicePool(const ServicePool&) = delete;
	ServicePool& operator=(const ServicePool&) = delete;
};

} // ember