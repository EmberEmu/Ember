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
#include "ConfigStore.h"
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
	const ConfigStore& cfg_store_;

	void accept_connection();
	void dispatch_socket();

public:
	NetworkListener(std::string_view interface, std::uint16_t port, bool tcp_no_delay,
	                thread::ServicePool& pool, ClientBuilder builder, SessionManager& sessions,
	                const ConfigStore& cfg_store, log::Logger& logger);

	~NetworkListener();

	std::uint16_t port() const;
	void shutdown();
};

} // realm, ember