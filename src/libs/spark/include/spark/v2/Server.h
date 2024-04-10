/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/PeerHandler.h>
#include <spark/v2/RemotePeer.h>
#include <spark/v2/HandlerRegistry.h>
#include <logger/Logging.h>
#include <boost/asio/io_context.hpp>
#include <boost/uuid/uuid.hpp>
#include <expected>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace ember::spark::v2 {

class Server final {
	boost::asio::io_context& ctx_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::resolver resolver_;
	std::vector<std::unique_ptr<RemotePeer>> peers_;
	HandlerRegistry handlers_;
	std::string name_;
	log::Logger* logger_;
	
	boost::asio::awaitable<void> listen();
	boost::asio::awaitable<void> accept_connection();
	boost::asio::awaitable<void> accept(boost::asio::ip::tcp::socket socket);
	boost::asio::awaitable<bool> connect(const std::string& host, std::uint16_t port);
	boost::asio::awaitable<void> open_channel(std::string host, const std::uint16_t port,
	                                          std::string service, Handler* handler);

public:
	Server(boost::asio::io_context& context, const std::string& name,
	       const std::string& iface, std::uint16_t port, log::Logger* logger);

	void register_handler(Handler* handler);
	void deregister_handler(Handler* handler);

	void connect(std::string host, std::uint16_t port, std::string service, Handler* handler);
	void shutdown();
};

} // spark, ember