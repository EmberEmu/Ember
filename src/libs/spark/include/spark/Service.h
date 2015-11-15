/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Router.h>
#include <spark/SessionManager.h>
#include <spark/Listener.h>
#include <logger/Logger.h>
#include <boost/asio.hpp>
#include <string>
#include <cstdint>

namespace ember { namespace spark {

class Service {
	boost::asio::io_service& service_;
	boost::asio::signal_set signals_;

	Router router_;
	Listener listener_;
	SessionManager sessions_;
	log::Logger* logger_;
	log::Filter filter_;
	
	void shutdown();

public:
	Service(boost::asio::io_service& service, const std::string& interface, std::uint16_t port,
	        log::Logger* logger, log::Filter filter);

	void connect(const std::string& host, std::uint16_t port);
	Router* router();
};

}} // spark, ember