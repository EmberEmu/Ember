/*
 * Copyright (c) 2016 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "WorldClients.h"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string_view>
#include <cstdint>

namespace ember::realm {

class WorldConnection final {
	bool stopped_;

	boost::asio::ip::tcp::socket socket_;

	boost::asio::awaitable<void> resolve(boost::asio::io_context& service, std::string host, std::string port);
	boost::asio::awaitable<void> connect(const boost::asio::ip::tcp::resolver::results_type& results);
	boost::asio::awaitable<void> receive();
	boost::asio::awaitable<void> send();

public:
	WorldConnection(boost::asio::io_context& service, std::string host, std::string port);
	~WorldConnection();

	void shutdown();
};

} // realm, ember