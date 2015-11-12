/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logger.h>
#include <boost/asio.hpp>
#include <string>
#include <cstdint>

namespace ember { namespace spark {

class Service {
	boost::asio::io_service& service_;
	log::Logger* logger_;
	log::Filter filter_;

public:
	Service(boost::asio::io_service& service, std::string host, std::uint16_t port,
	        log::Logger* logger, log::Filter filter);
};

}} // spark, ember