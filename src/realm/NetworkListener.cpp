/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "NetworkListener.h"
#include <logger/Logger.h>
#include "FilterTypes.h"
#include "ClientConnection.h"
#include <boost/asio/dispatch.hpp>
#include <functional>
#include <memory>
#include <utility>

namespace ember::realm {

void NetworkListener::accept_connection() {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	if(!acceptor_.is_open()) {
		return;
	}

	acceptor_.async_accept(socket_, [this](const boost::system::error_code& ec) {
		if(ec == boost::asio::error::operation_aborted) {
			return;
		}

		if(!ec) {
			dispatch_socket();
		} else {
			LOG_DEBUG_ASYNC(logger_, "Unable to accept connection, {}", ec.message());
		}

		++index_;
		index_ %= pool_.size();
		socket_ = tcp_socket(pool_.get(index_));
		accept_connection();
	});
}

void NetworkListener::dispatch_socket() {
	boost::system::error_code ec{};
	const auto ep = socket_.remote_endpoint(ec);

	if(!ec) {
		LOG_DEBUG_ASYNC(logger_, "Accepted connection from {}", ep.address().to_string());
		auto executor = socket_.get_executor();

		boost::asio::dispatch(executor,
		                      [&, socket = std::move(socket_), i = index_]() mutable {
			sessions_.emplace(sessions_, std::move(socket), ClientIdent(i), logger_);
		});
	} else {
		LOG_DEBUG_ASYNC(logger_, "Aborted connection, remote peer disconnected");
	}
}

void NetworkListener::shutdown() {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;
	acceptor_.close();
	sessions_.stop_all();
}

std::uint16_t NetworkListener::port() const {
	return acceptor_.local_endpoint().port();
}

NetworkListener::~NetworkListener() {
	shutdown();
}

} // realm, ember