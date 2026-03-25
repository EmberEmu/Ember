/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "NetworkListener.h"
#include "FilterTypes.h"
#include <logger/Logger.h>
#include <boost/asio/dispatch.hpp>
#include <functional>
#include <memory>
#include <utility>

namespace ember::realm {

void NetworkListener::accept_connection() {
	LOG_TRACE(logger_, log_func);

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
			LOG_DEBUG(logger_, "Unable to accept connection, {}", ec.message());
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
		LOG_DEBUG(logger_, "Accepted connection from {}", ep.address().to_string());
		auto executor = socket_.get_executor();

		boost::asio::dispatch(executor, [&, socket = std::move(socket_), i = index_]() mutable {
			auto client = builder_.create(std::move(socket), ClientIdent(i));
			sessions_.start(std::move(client));
		});
	} else {
		LOG_DEBUG(logger_, "Aborted connection, remote peer disconnected");
	}
}

void NetworkListener::shutdown() {
	bool expected = false;

	if(!stopped_.compare_exchange_strong(expected, true)) {
		return;
	}

	LOG_TRACE(logger_, log_func);
	acceptor_.close();
}

std::uint16_t NetworkListener::port() const {
	return acceptor_.local_endpoint().port();
}

NetworkListener::~NetworkListener() {
	shutdown();
}

} // realm, ember