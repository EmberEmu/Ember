/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/Server.h>

namespace ember::spark::v2 {

Server::Server(boost::asio::io_context& context, const std::string& iface,
               const std::uint16_t port, log::Logger* logger)
	: context_(context), logger_(logger), listener_(*this, context, iface, port, true, logger) {}

void Server::accept(boost::asio::ip::tcp::socket socket) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
}

void Server::register_service(spark::v2::Service* service) {

}

void Server::connect(const std::string& host, const std::uint16_t port) {

}

void Server::connect(const std::string& address) {

}

void Server::shutdown() {
	listener_.shutdown();
}

} // spark, ember