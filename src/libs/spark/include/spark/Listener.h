/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logging.h>
#include <boost/asio.hpp>

namespace ember { namespace spark {

struct Link;
class LinkMap;
class SessionManager;
class HandlerMap;
class ServicesMap;

class Listener {
	boost::asio::io_service& service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::socket socket_;

	SessionManager& sessions_;
	log::Logger* logger_;
	log::Filter filter_;
	const Link& link_;
	const HandlerMap& handlers_;
	ServicesMap& services_;

	void accept_connection();
	void start_session(boost::asio::ip::tcp::socket socket);

public:
	Listener(boost::asio::io_service& service, std::string interface, std::uint16_t port,
	         SessionManager& sessions, const HandlerMap& handlers, ServicesMap& services,
	         const Link& link, log::Logger* logger, log::Filter filter);

	void shutdown();
};

}} // spark, ember