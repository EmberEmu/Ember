/*
 * Copyright (c) 2021 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/Server.h>
#include <spark/Handler.h>
#include <spark/Connection.h>
#include <spark/buffers/BufferAdaptor.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/Utility.h>
#include <logger/Logger.h>
#include <shared/FilterTypes.h>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/detached.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <format>
#include <memory>

namespace ember::spark {

namespace asio = boost::asio;

Server::Server(boost::asio::io_context& context, std::string_view name,
               std::string_view iface, const std::uint16_t port, log::Logger& logger)
	: ctx_(context),
	  acceptor_(context, asio::ip::tcp::endpoint(asio::ip::make_address(iface), port)),
	  resolver_(context),
	  logger_(logger),
	  stopped_(false) {
	acceptor_.set_option(asio::ip::tcp::no_delay(true));
	acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));

	// generate random unique name for this service
	const auto uuid = boost::uuids::random_generator()();
	name_ = std::format("{}:{}", name, boost::uuids::to_string(uuid));
	asio::co_spawn(ctx_, listen(), asio::detached);
}

asio::awaitable<void> Server::listen() {
	LOG_TRACE(logger_, log_func);

	while(acceptor_.is_open()) {
		co_await accept_connection();
	}
}

asio::awaitable<void> Server::accept_connection() {
	LOG_TRACE(logger_, log_func);

	asio::ip::tcp::socket socket(ctx_);
	auto [ec] = co_await acceptor_.async_accept(socket, boost::asio::as_tuple);

	if(ec) {
		co_return;
	}

	const auto ep = socket.remote_endpoint(ec);

	if(ec) {
		LOG_DEBUG(logger_, "[spark] Unable to obtain endpoint, remote peer disconnected");
		co_return;
	}

	LOG_DEBUG(logger_, "[spark] Accepted connection from {}:{}", ep.address().to_string(), ep.port());
	co_await accept(std::move(socket));
}

asio::awaitable<void> Server::accept(boost::asio::ip::tcp::socket socket) try {
	LOG_TRACE(logger_, log_func);

	auto ep = socket.remote_endpoint();
	const auto key = std::format("{}:{}", ep.address().to_string(), std::to_string(ep.port()));

	Connection connection(std::move(socket), logger_, [this, key]() {
		close_peer(key);
	});

	auto banner = co_await receive_banner(connection);
	co_await send_banner(connection, name_);

	LOG_INFO(logger_, "[spark] Connected to {}", banner);

	auto peer = std::make_shared<RemotePeer>(
		ctx_, std::move(connection), name_, banner, handlers_, logger_
	);

	peer->start();
	peers_.add(key, std::move(peer));
} catch(const std::exception& e) {
	LOG_WARN(logger_, e.what());
}

void Server::register_handler(gsl::not_null<Handler*> handler) {
	handlers_.register_service(handler);

	LOG_TRACE(logger_, "[spark] Registered handler for {}", handler->type());
}

void Server::deregister_handler(gsl::not_null<Handler*> handler) {
	handlers_.deregister_service(handler);
	peers_.notify_remove_handler(handler);

	LOG_TRACE(logger_, "[spark] Removed handler for {}", handler->type());
}

asio::awaitable<std::shared_ptr<RemotePeer>>
Server::connect(const std::string_view host, const std::uint16_t port) try {
	LOG_TRACE(logger_, log_func);

	auto results = co_await resolver_.async_resolve(host, std::to_string(port));

	asio::ip::tcp::socket socket(ctx_);
	co_await asio::async_connect(socket, results.begin(), results.end());

	const auto key = std::format("{}:{}", host, port);

	Connection connection(std::move(socket), logger_, [this, key]() {
		close_peer(key);
	});

	co_await send_banner(connection, name_);
	auto banner = co_await receive_banner(connection);

	LOG_INFO(logger_, "[spark] Connected to {}", banner);

	auto peer = std::make_shared<RemotePeer>(
		ctx_, std::move(connection), name_, banner, handlers_, logger_
	);

	co_return peer;
} catch(const std::exception& e) {
	LOG_DEBUG(logger_, "[spark] Could not connect to {}:{} ({})", host, port, e.what());
	co_return nullptr;
}

asio::awaitable<void> Server::try_open(std::string host, std::uint16_t port,
                                     std::string service, gsl::not_null<Handler*> handler) {
	LOG_TRACE(logger_, log_func);

	const auto key = std::format("{}:{}", host, port);

	// check if we've already established a connection with this peer
	if(const auto& peer = peers_.find(key); peer) {
		peer->open_channel(std::move(service), handler);
		co_return;
	}

	// open a connection if we didn't find one
	auto peer = co_await connect(host, port);

	if(!peer) {
		handler->connect_failed(host, port);
		co_return;
	}

	peer->start();
	peer->open_channel(std::move(service), handler);
	peers_.add(key, std::move(peer));
}

void Server::connect(const std::string_view host, const std::uint16_t port,
                     std::string_view service, gsl::not_null<Handler*> handler) {
	connect(std::string(host), port, std::string(service), handler);
}

void Server::connect(std::string host, const std::uint16_t port,
                     std::string service, gsl::not_null<Handler*> handler) {
	LOG_TRACE(logger_, log_func);

	asio::co_spawn(ctx_, try_open(
		std::move(host), port, std::move(service), handler), asio::detached
	);
}

asio::awaitable<void> Server::send_banner(Connection& conn, const std::string& banner) {
	LOG_TRACE(logger_, log_func);

	core::HelloT hello {
		.description = banner
	};

	Message msg;
	finish(hello, msg);
	write_header(msg);
	co_await conn.send(msg);
}


asio::awaitable<std::string> Server::receive_banner(Connection& conn) {
	LOG_TRACE(logger_, log_func);

	auto msg = co_await conn.receive_msg();
	spark::io::BufferAdaptor adaptor(msg);
	spark::io::BinaryStream stream(adaptor);

	MessageHeader header;

	if(header.read_from_stream(stream) != MessageHeader::State::ok
	   || header.size <= stream.total_read()) {
		throw exception("bad message header");
	}

	const auto header_size = stream.total_read();
	std::span flatbuffer(msg.data() + header_size, msg.size_bytes() - header_size);

	flatbuffers::Verifier verifier(flatbuffer.data(), flatbuffer.size());
	auto fb = core::GetHeader(flatbuffer.data());
	auto hello = fb->message_as_Hello();

	if(!hello || !hello->Verify(verifier)) {
		throw exception("bad flatbuffer message");
	}

	co_return hello->description()->str();
}

// todo, need to link down to all channels, error code
void Server::close_peer(const std::string& key) {
	//LOG_TRACE_ASYNC(logger_, log_func);
	//peers_.remove(key);

	//LOG_DEBUG_FILTER(logger_, LF_SPARK)
	//	<< "[spark] Close connection " << key << LOG_ASYNC;
}

void Server::shutdown() {
	if(stopped_) {
		return;
	}

	LOG_DEBUG(logger_, "[spark] Service shutting down...");

	acceptor_.close();
	stopped_ = true;
}

Server::~Server() {
	shutdown();
}

std::uint16_t Server::port() const {
	return acceptor_.local_endpoint().port();
}

} // spark, ember