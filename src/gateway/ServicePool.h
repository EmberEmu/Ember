/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <cstdint>

namespace ember {

class ServicePool final {
	std::size_t next_service_;
	std::size_t pool_size_;
	std::vector<std::shared_ptr<boost::asio::io_service::work>> work_;
	std::vector<std::shared_ptr<boost::asio::io_service>> services_;

public:
	explicit ServicePool(std::size_t pool_size);
	
	boost::asio::io_service& get_service();
	void run();
	void stop();
	std::size_t size() const;

	ServicePool(const ServicePool&) = delete;
	ServicePool& operator=(const ServicePool&) = delete;
};

} // ember