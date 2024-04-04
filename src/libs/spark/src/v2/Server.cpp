/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/Server.h>
#include <spark/v2/Handler.h>
#include <spark/v2/PeerConnection.h>
#include <shared/FilterTypes.h>
#include <boost/asio.hpp>
#include <format>
#include <memory>

namespace ember::spark::v2 {

namespace ba = boost::asio;

Server::Server(boost::asio::io_context& context, const std::string& iface,
               const std::uint16_t port, log::Logger* logger)
	: ctx_(context),
	  acceptor_(ctx_, ba::ip::tcp::endpoint(ba::ip::address::from_string(iface), port)),
	  resolver_(ctx_),
	  logger_(logger) {
	acceptor_.set_option(ba::ip::tcp::no_delay(true));
	acceptor_.set_option(ba::ip::tcp::acceptor::reuse_address(true));
	ba::co_spawn(ctx_, listen(), ba::detached);
}

ba::awaitable<void> Server::listen() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	while(acceptor_.is_open()) {
		auto result = co_await accept_connection();

		if(result) {
			accept(std::move(*result));
		}
	}
}

ba::awaitable<Server::SocketReturn> Server::accept_connection() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	ba::ip::tcp::socket socket(ctx_);

	auto [ec] = co_await acceptor_.async_accept(socket, as_tuple(ba::use_awaitable));

	if(ec) {
		co_return std::unexpected(ec);
	}

	const auto ep = socket.remote_endpoint(ec);

	if(ec) {
		LOG_DEBUG(logger_)
			<< "[spark] Aborted connection, remote peer disconnected"
			<< LOG_ASYNC;
		co_return std::unexpected(ec);
	}

	LOG_DEBUG(logger_)
		<< "[spark] Accepted connection "
		<< ep.address().to_string()
		<< ":" << ep.port()
		<< LOG_ASYNC;

	co_return socket;
}

void Server::accept(boost::asio::ip::tcp::socket socket) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
	auto peer = std::make_unique<RemotePeer>(std::move(socket), handlers_, logger_);
	peers_.emplace_back(std::move(peer));
}

void Server::register_handler(spark::v2::Handler* handler) {
	LOG_DEBUG_FILTER(logger_, LF_SPARK)
		<< "[spark] Registered handler for "
		<< handler->type()
		<< LOG_ASYNC;
	handlers_.register_service(handler);
}

void Server::deregister_handler(spark::v2::Handler* handler) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

}

ba::awaitable<void> Server::do_connect(const std::string host, const std::uint16_t port) {
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

		auto peer = std::make_unique<RemotePeer>(std::move(socket), handlers_, logger_);
		peers_.emplace_back(std::move(peer));
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
	acceptor_.close();
}

} // spark, ember