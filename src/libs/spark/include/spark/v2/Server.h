/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Service.h>
#include <spark/v2/NetworkListener.h>
#include <spark/v2/SocketAcceptor.h>
#include <spark/v2/PeerHandler.h>
#include <spark/v2/RemotePeer.h>
#include <logger/Logging.h>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace ember::spark::v2 {

class Server final : public SocketAcceptor {
	boost::asio::io_context& ctx_;
	boost::asio::ip::tcp::resolver resolver_;
	NetworkListener listener_;
	std::vector<std::unique_ptr<RemotePeer>> handlers_;
	log::Logger* logger_;
	
	void accept(boost::asio::ip::tcp::socket socket);
	boost::asio::awaitable<void> do_connect(const std::string& host, const std::uint16_t port);

public:
	Server(boost::asio::io_context& context, const std::string& iface,
	        std::uint16_t port, log::Logger* logger);

	void register_service(spark::v2::Service* service);
	void deregister_service(spark::v2::Service* service);

	void connect(const std::string& host, const std::uint16_t port);
	void shutdown();
};

} // spark, ember