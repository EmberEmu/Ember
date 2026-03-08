/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WorldConnection.h"
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <utility>
#include <vector>

namespace ember::gateway {

namespace asio = boost::asio;

WorldConnection::WorldConnection(asio::io_context& service, std::string host, std::string port)
	: socket_(service)
	, stopped_(false) {
	asio::co_spawn(service, resolve(service, std::move(host), std::move(port)), asio::detached);
}

WorldConnection::~WorldConnection() {
	shutdown();
}

void WorldConnection::shutdown() {
	if(stopped_) {
		return;
	}

	socket_.close();
}

asio::awaitable<void>
WorldConnection::resolve(asio::io_context& service, std::string host, std::string port) {
	asio::ip::tcp::resolver resolver(service);
	const auto& [ar_ec, endpoints] = co_await resolver.async_resolve(host, port, asio::as_tuple);

	if(ar_ec) {
		co_return;
	}

	co_await connect(endpoints);
}

asio::awaitable<void> WorldConnection::connect(const asio::ip::tcp::resolver::results_type& results) {
	const auto& [ec, _] = co_await asio::async_connect(socket_, results, asio::as_tuple);

	if(ec) {
		co_return;
	}

	co_await receive();
}

asio::awaitable<void> WorldConnection::receive() {
	std::vector<char> buffer;

	while(stopped_) {
		const auto& [ec, bytes] = co_await socket_.async_receive(asio::buffer(buffer), asio::as_tuple);

		if(ec) {
			co_return;
		}
	}
}

asio::awaitable<void> WorldConnection::send() {
	co_return;
}

} // gateway, ember