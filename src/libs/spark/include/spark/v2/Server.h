/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/Service.h>
#include <spark/v2/NetworkListener.h>
#include <spark/v2/SocketAcceptor.h>
#include <logger/Logging.h>
#include <boost/asio/io_context.hpp>
#include <string>
#include <cstdint>

namespace ember::spark::v2 {

class Server final : public SocketAcceptor {
	boost::asio::io_context& context_;
	log::Logger* logger_;
	NetworkListener listener_;

	void accept(boost::asio::ip::tcp::socket socket);

public:
	Server(boost::asio::io_context& context, const std::string& iface,
	        std::uint16_t port, log::Logger* logger);

	void register_service(spark::v2::Service* service);
	void connect(const std::string& host, const std::uint16_t port);
	void connect(const std::string& address);
	void shutdown();
};

} // spark, ember