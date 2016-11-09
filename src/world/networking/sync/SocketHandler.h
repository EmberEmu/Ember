/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/Logging.h>
#include <boost/asio.hpp>
#include <utility>

#define LF_NETWORK 1 // todo, remove

namespace ember {

class SocketHandler {
	boost::asio::ip::tcp::socket socket_;
	log::Logger* logger_;

	void read();

public:
	SocketHandler(boost::asio::ip::tcp::socket socket) : socket_(std::move(socket)) { }
	
	void start();
	void stop();

	void write();
};

} // ember