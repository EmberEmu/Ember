/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Listener.h>

namespace ember { namespace spark {

Listener::Listener(boost::asio::io_service& service, log::Logger* logger, log::Filter filter)
                   : service_(service), acceptor_(service), socket_(service),
                     signals_(service_, SIGINT, SIGTERM), logger_(logger), filter_(filter) {
	acceptor_.set_option(boost::asio::ip::tcp::no_delay(true));
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	signals_.async_wait(std::bind(&Listener::shutdown, this));
	accept_connection();
}

void Listener::accept_connection() {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
		if(!acceptor_.is_open()) {
			return;
		}

		if(!ec) {
			auto ip = socket_.remote_endpoint().address();

			LOG_DEBUG_FILTER(logger_, filter_)
				<< "Accepted connection " << ip.to_string() << ":"
				<< socket_.remote_endpoint().port() << LOG_ASYNC;

			//start_session(std::move(socket_));
		}

		accept_connection();
	});
}

void Listener::shutdown() {
	LOG_DEBUG_FILTER(logger_, filter_) << "Spark listener shutting down..." << LOG_ASYNC;
	acceptor_.close();
}

}} // spark, ember