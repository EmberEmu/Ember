/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Client.h>
#include <spark/SessionManager.h>
#include <spark/MessageHandler.h>
#include <spark/NetworkSession.h>

namespace ember { namespace spark {

Client::Client(boost::asio::io_service& service, bai::tcp::resolver::iterator endpoint_it,
               SessionManager& sessions, log::Logger* logger, log::Filter filter)
               : service_(service), socket_(service), sessions_(sessions), logger_(logger), filter_(filter) {
	connect(endpoint_it);
}

void Client::connect(bai::tcp::resolver::iterator endpoint_it) {
	boost::asio::async_connect(socket_, endpoint_it,
							   [this](boost::system::error_code ec, bai::tcp::resolver::iterator) {
		if(!ec) {
			start_session(std::move(socket_));
		}
	});
}

void Client::start_session(bai::tcp::socket socket) {
	MessageHandler m_handler(MessageHandler::Mode::CLIENT, logger_, filter_);
	auto session = std::make_shared<NetworkSession>(sessions_, std::move(socket), m_handler, logger_, filter_);
	sessions_.start(session);
}

}} // spark, ember