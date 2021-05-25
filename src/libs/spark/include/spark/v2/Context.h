/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Service.h>
#include <logger/Logging.h>
#include <boost/asio.hpp>
#include <string>
#include <cstdint>

namespace ember::spark::v2 {

class Context final {
	boost::asio::io_context& context_;
	log::Logger* logger_;

public:
	Context(boost::asio::io_context& context, const std::string& iface,
	        std::uint16_t port, log::Logger* logger);

	void register_service(spark::v2::Service* service);
	void shutdown();
};

} // spark, ember