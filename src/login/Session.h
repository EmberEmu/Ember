/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PacketBuffer.h"
#include <boost/asio.hpp>
#include <cstddef>

namespace ember {

template<typename T>
struct Session {
	PacketBuffer buffer;
	T handler;
	boost::asio::ip::tcp::socket socket;
	boost::asio::strand strand;
	boost::asio::deadline_timer timer;

	Session(T handler, boost::asio::ip::tcp::socket socket,
	        boost::asio::io_service& service) : handler(std::move(handler)),
	        timer(service), strand(service), socket(std::move(socket)) {}
};

} // ember