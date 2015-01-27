/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio.hpp>
#include <memory>

namespace ember {

class LoginHandler : public std::enable_shared_from_this<LoginHandler> {
	boost::asio::ip::tcp::socket socket_;

public:
	LoginHandler(boost::asio::ip::tcp::socket socket, boost::asio::io_service&)
	             : socket_(std::move(socket)) { }
	~LoginHandler();

	void start();
};

} //ember