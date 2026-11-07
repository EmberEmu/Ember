/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "NetworkListener.h"
#include "ClientSlots.h"
#include "FilterTypes.h"
#include <logger/Logger.h>
#include <boost/asio/dispatch.hpp>
#include <functional>
#include <memory>
#include <utility>

namespace ember::realm {

NetworkListener::NetworkListener(std::string_view interface, std::uint16_t port, bool tcp_no_delay,
                                 thread::ServicePool& pool, ClientBuilder builder,
                                 SessionManager& sessions, const ConfigStore& cfg_store, log::Logger& logger)
	: sessions_(sessions)
	, builder_(std::move(builder))
	, acceptor_(pool.get_next(), asio::ip::tcp::endpoint(asio::ip::make_address(interface), port))
	, index_(0)
	, socket_(pool.get(index_))
	, pool_(pool)
	, logger_(logger)
	, stopped_(false)
	, cfg_store_(cfg_store) {
	acceptor_.set_option(asio::ip::tcp::no_delay(tcp_no_delay));
	acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
	accept_connection();
}

void NetworkListener::accept_connection() {
	LOG_TRACE(logger_, log_func);

	if(!acceptor_.is_open()) {
		return;
	}

	acceptor_.async_accept(socket_, [this](const boost::system::error_code& ec) {
		if(ec == boost::asio::error::operation_aborted) {
			return;
		}

		if(!ec) {
			dispatch_socket();
		} else {
			LOG_DEBUG(logger_, "Unable to accept connection, {}", ec.message());
		}

		++index_;
		index_ %= pool_.size();
		socket_ = tcp::Socket(pool_.get(index_));
		accept_connection();
	});
}

void NetworkListener::dispatch_socket() {
	boost::system::error_code ec;
	const auto ep = socket_.remote_endpoint(ec);

	if(ec) {
		LOG_DEBUG(logger_, "Aborted connection: {}", ec.what());
		return;
	}

	const auto max_socks = cfg_store_.config().max_sockets; // todo, this should not be here

	if(!ClientSlots::reserve_slot(max_socks)) {
		LOG_WARN(logger_, "Rejected connection from {}, too many sockets", ep.address().to_string());
		return;
	}

	LOG_DEBUG(logger_, "Accepted connection from {}", ep.address().to_string());
	auto& executor = socket_.get_executor();

	/*
	 * This dispatch ensures that the client is created on the same thread that it will live on,
	 * allowing the thread local allocators to operate in 'unsafe entrant' mode, meaning they
	 * can forego ensuring that the calling thread has previously initialised the allocator.
	 */
	boost::asio::dispatch(executor, [this, socket = std::move(socket_), ep]() mutable {
		sessions_.enqueue(
			builder_.create(std::move(socket), ep)
		);
	});
}

void NetworkListener::shutdown() {
	if(stopped_.exchange(true)) {
		return;
	}

	LOG_TRACE(logger_, log_func);
	acceptor_.close();
}

std::uint16_t NetworkListener::port() const {
	return acceptor_.local_endpoint().port();
}

NetworkListener::~NetworkListener() {
	shutdown();
}

} // realm, ember