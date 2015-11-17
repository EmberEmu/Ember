/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Listener.h>
#include <spark/NetworkSession.h>
#include <spark/SessionManager.h>

namespace ember { namespace spark {

Listener::Listener(boost::asio::io_service& service, std::string interface, std::uint16_t port, 
                   SessionManager& sessions, const HandlerMap& handlers, const Link& link,
                   log::Logger* logger, log::Filter filter)
                   : service_(service), acceptor_(service, boost::asio::ip::tcp::endpoint(
                     boost::asio::ip::address::from_string(interface), port)), link_(link),
                     socket_(service), sessions_(sessions), logger_(logger), filter_(filter),
                     handlers_(handlers) {
	acceptor_.set_option(boost::asio::ip::tcp::no_delay(true));
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
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
				<< "[spark] Accepted connection from " << ip.to_string() << ":"
				<< socket_.remote_endpoint().port() << LOG_ASYNC;

			start_session(std::move(socket_));
		}

		accept_connection();
	});
}

void Listener::start_session(boost::asio::ip::tcp::socket socket) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;
	MessageHandler m_handler(handlers_, link_, false, logger_, filter_);
	auto session = std::make_shared<NetworkSession>(sessions_, std::move(socket), m_handler, logger_, filter_);
	sessions_.start(session);
}

void Listener::shutdown() {
	LOG_DEBUG_FILTER(logger_, filter_) << "[spark] Listener shutting down..." << LOG_ASYNC;
	acceptor_.close();
}

}} // spark, ember