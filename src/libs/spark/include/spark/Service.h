/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/CoreHandler.h>
#include <spark/HandlerMap.h>
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
	boost::asio::ip::tcp::socket socket_;

	Listener listener_;
	SessionManager sessions_;
	CoreHandler core_handler_;
	HandlerMap handlers_;
	log::Logger* logger_;
	log::Filter filter_;
	
	void start_session(boost::asio::ip::tcp::socket socket);
	void default_handler(ember::messaging::MessageRoot* message);
	void shutdown();

public:
	Service(boost::asio::io_service& service, const std::string& interface, std::uint16_t port,
	        log::Logger* logger, log::Filter filter);

	void connect(const std::string& host, std::uint16_t port);
};

}} // spark, ember