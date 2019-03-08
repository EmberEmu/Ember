/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logging.h>
#include <boost/asio.hpp>

namespace ember::spark {

struct Link;
class LinkMap;
class SessionManager;
class EventDispatcher;
class ServicesMap;

class Listener {
	boost::asio::io_context& service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::socket socket_;

	SessionManager& sessions_;
	log::Logger* logger_;
	const Link& link_;
	const EventDispatcher& handlers_;
	ServicesMap& services_;

	void accept_connection();
	void start_session(boost::asio::ip::tcp::socket socket);

public:
	Listener(boost::asio::io_context& service, std::string interface, std::uint16_t port,
	         SessionManager& sessions, const EventDispatcher& handlers, ServicesMap& services,
	         const Link& link, log::Logger* logger);

	void shutdown();
};

} // spark, ember