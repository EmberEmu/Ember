/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/Server.h>
#include <spark/v2/PeerConnection.h>
#include <shared/FilterTypes.h>
#include <boost/asio/ip/tcp.hpp>
#include <format>
#include <memory>

namespace ember::spark::v2 {

namespace ba = boost::asio;

Server::Server(boost::asio::io_context& context, const std::string& iface,
               const std::uint16_t port, log::Logger* logger)
	: ctx_(context),
	  resolver_(ctx_),
	  logger_(logger),
	  listener_(*this, context, iface, port, true, logger) {}

void Server::accept(boost::asio::ip::tcp::socket socket) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
	auto peer = std::make_unique<RemotePeer>(std::move(socket), false, logger_);
	handlers_.emplace_back(std::move(peer));
}

void Server::register_service(spark::v2::Service* service) {
	LOG_DEBUG_FILTER(logger_, LF_SPARK)
		<< "[spark] Registered service, "
		<< service->service_type()
		<< LOG_ASYNC;
}

void Server::deregister_service(spark::v2::Service* service) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

}

ba::awaitable<void> Server::do_connect(const std::string& host, const std::uint16_t port) {
	auto results = co_await resolver_.async_resolve(
		host, std::to_string(port), ba::use_awaitable
	);

	ba::ip::tcp::socket socket(ctx_);

	auto [ec, ep] = co_await ba::async_connect(
		socket, results.begin(), results.end(), as_tuple(ba::use_awaitable)
	);

	if(!ec) {
		LOG_DEBUG_FILTER(logger_, LF_SPARK)
			<< std::format("[spark] Connected to {}:{}", host, port)
			<< LOG_ASYNC;

		auto peer = std::make_unique<RemotePeer>(std::move(socket), true, logger_);
		peer.release(); // temp
	} else {
		const auto msg = std::format("[spark] Could not connect to {}:{}", host, port);
		LOG_DEBUG_FILTER(logger_, LF_SPARK) << msg << LOG_ASYNC;
	}
}

void Server::connect(const std::string& host, const std::uint16_t port) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
	ba::co_spawn(ctx_, do_connect(host, port), ba::detached);
}

void Server::shutdown() {
	LOG_DEBUG_FILTER(logger_, LF_SPARK) << "[spark] Service shutting down..." << LOG_ASYNC;
	listener_.shutdown();
}

} // spark, ember