/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SessionManager.h"
#include "ClientBuilder.h"
#include "SocketType.h"
#include <logger/LoggerFwd.h>
#include <shared/ClientIdent.h>
#include <thread/ServicePool.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <atomic>
#include <string>
#include <string_view>
#include <cstddef>


namespace ember::realm {

namespace asio = boost::asio;

class NetworkListener final {
	SessionManager& sessions_;
	ClientBuilder builder_;
	tcp_acceptor acceptor_;
	std::size_t index_;
	tcp_socket socket_;
	thread::ServicePool& pool_;
	log::Logger& logger_;
	std::atomic_bool stopped_;

	void accept_connection();
	void dispatch_socket();

public:
	NetworkListener(thread::ServicePool& pool, SessionManager& sessions, std::string_view interface,
	                std::uint16_t port, bool tcp_no_delay, log::Logger& logger)
		: sessions_(sessions)
		, builder_(logger)
		, acceptor_(
			pool.get_next(),
			asio::ip::tcp::endpoint(asio::ip::make_address(interface), port)
		  )
		, index_(0)
		, socket_(pool.get(index_))
		, pool_(pool)
		, logger_(logger)
		, stopped_(false) {
		acceptor_.set_option(asio::ip::tcp::no_delay(tcp_no_delay));
		acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
		accept_connection();
	}

	~NetworkListener();

	std::uint16_t port() const;
	void shutdown();
};

} // realm, ember