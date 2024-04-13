/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/Server.h>
#include <spark/v2/RemotePeer.h>
#include <spark/v2/Handler.h>
#include <spark/v2/PeerConnection.h>
#include <shared/FilterTypes.h>
#include <boost/asio.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <format>
#include <memory>

namespace ember::spark::v2 {

namespace ba = boost::asio;

Server::Server(boost::asio::io_context& context, const std::string& name,
               const std::string& iface, const std::uint16_t port, log::Logger* logger)
	: ctx_(context),
	  acceptor_(ctx_, ba::ip::tcp::endpoint(ba::ip::address::from_string(iface), port)),
	  resolver_(ctx_),
	  logger_(logger) {
	acceptor_.set_option(ba::ip::tcp::no_delay(true));
	acceptor_.set_option(ba::ip::tcp::acceptor::reuse_address(true));
	const auto uuid = boost::uuids::random_generator()();
	name_ = std::format("{}:{}", name, boost::uuids::to_string(uuid));
	ba::co_spawn(ctx_, listen(), ba::detached);
}

ba::awaitable<void> Server::listen() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	while(acceptor_.is_open()) {
		co_await accept_connection();
	}
}

ba::awaitable<void> Server::accept_connection() {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	ba::ip::tcp::socket socket(ctx_);
	auto [ec] = co_await acceptor_.async_accept(socket, as_tuple(ba::use_awaitable));

	if(ec) {
		LOG_DEBUG(logger_)
			<< "[spark] Unable to accept connection"
			<< LOG_ASYNC;
		co_return;
	}

	const auto ep = socket.remote_endpoint();

	if(ec) {
		LOG_DEBUG(logger_)
			<< "[spark] Unable to obtain endpoint, remote peer disconnected"
			<< LOG_ASYNC;
		co_return;
	}

	LOG_DEBUG_FILTER(logger_, LF_SPARK)
		<< "[spark] Accepted connection "
		<< ep.address().to_string()
		<< ":" << ep.port()
		<< LOG_ASYNC;

	co_await accept(std::move(socket));
}

ba::awaitable<void> Server::accept(boost::asio::ip::tcp::socket socket) try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto ep = socket.remote_endpoint();
	auto peer = std::make_shared<RemotePeer>(std::move(socket), handlers_, logger_);
	auto banner = co_await peer->receive_banner();
	co_await peer->send_banner(name_);

	LOG_INFO_FILTER(logger_, LF_SPARK)
		<< std::format("[spark] Connected to {}", banner)
		<< LOG_ASYNC;

	const auto key = std::format("{}:{}", ep.address().to_string(), std::to_string(ep.port()));
	peers_.add(key, peer);
	peer->start();
} catch(const std::exception& e) {
	LOG_WARN_FILTER(logger_, LF_SPARK) << e.what() << LOG_ASYNC;
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
	handlers_.deregister_service(handler);
	peers_.notify_remove_handler(handler);
}

ba::awaitable<bool> Server::connect(const std::string& host, const std::uint16_t port) try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto results = co_await resolver_.async_resolve(host, std::to_string(port), ba::use_awaitable);
	ba::ip::tcp::socket socket(ctx_);

	co_await ba::async_connect(socket, results.begin(), results.end(), ba::use_awaitable);
	auto peer = std::make_shared<RemotePeer>(std::move(socket), handlers_, logger_);
	co_await peer->send_banner(name_);
	auto banner = co_await peer->receive_banner();

	LOG_INFO_FILTER(logger_, LF_SPARK)
		<< std::format("[spark] Connected to {}", banner)
		<< LOG_ASYNC;

	peers_.add(std::format("{}:{}", host, port), peer);
	peer->start();
	co_return true;
} catch(const std::exception& e) {
	const auto msg = std::format(
		"[spark] Could not connect to {}:{} ()", host, port, e.what()
	);

	LOG_WARN_FILTER(logger_, LF_SPARK) << msg << LOG_ASYNC;
	co_return false;
}

ba::awaitable<std::shared_ptr<RemotePeer>>
Server::find_or_connect(const std::string& host, const std::uint16_t port) {
	const auto key = std::format("{}:{}", host, port);
	auto peer = peers_.find(key);

	if(!peer) {
		const bool result = co_await connect(host, port);

		if(result) {
			co_return peers_.find(key);
		}
	}

	co_return nullptr;
}

ba::awaitable<void> Server::open_channel(std::string host, const std::uint16_t port,
                                         std::string service, Handler* handler) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
	auto peer = co_await find_or_connect(host, port);
	
	if(!peer) {
		co_return;
	}

	peer->open_channel(service, handler);
}

void Server::connect(std::string host, const std::uint16_t port,
                     std::string service, Handler* handler) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	ba::co_spawn(ctx_, open_channel(
		std::move(host), port, std::move(service), handler), ba::detached
	);
}

void Server::shutdown() {
	LOG_DEBUG_FILTER(logger_, LF_SPARK) << "[spark] Service shutting down..." << LOG_ASYNC;
	acceptor_.close();
}

} // spark, ember