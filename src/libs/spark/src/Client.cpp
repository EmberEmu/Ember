/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Client.h>

namespace ember { namespace spark {

Client::Client(boost::asio::io_service& service, bai::tcp::resolver::iterator endpoint_it) 
               : service_(service), socket_(service) {
	connect(endpoint_it);
}

void Client::connect(bai::tcp::resolver::iterator endpoint_it) {
	boost::asio::async_connect(socket_, endpoint_it,
							   [this](boost::system::error_code ec, bai::tcp::resolver::iterator) {
		if(!ec) {
			// start handshake
		}
	});
}

}} // spark, ember