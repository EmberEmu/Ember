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

namespace bai = boost::asio::ip;

class Client {
	boost::asio::io_service& service_;
	bai::tcp::socket socket_;

	void connect(bai::tcp::resolver::iterator endpoint_it);

public:
	Client(boost::asio::io_service& service, bai::tcp::resolver::iterator endpoint_it);
};

}} // spark, ember