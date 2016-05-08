/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "FilterTypes.h"
#include "ServicePool.h"
#include "SessionManager.h"
#include "ClientConnection.h"
#include <logger/Logger.h>
#include <shared/memory/ASIOAllocator.h>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <utility>
#include <cstdint>
#include <cstddef>

namespace ember {

namespace bai = boost::asio::ip;

class NetworkListener {
	boost::asio::signal_set signals_;
	boost::asio::ip::tcp::acceptor acceptor_;

	SessionManager sessions_;
	ServicePool& pool_;
	log::Logger* logger_;
	ASIOAllocator allocator_; // todo - thread_local, VS2015
	std::shared_ptr<ClientConnection> next_connection_;

	void accept_connection() {
		LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

		next_connection_ = std::make_shared<ClientConnection>(sessions_, pool_.get_service(), logger_);

		acceptor_.async_accept(next_connection_->socket(), [this](boost::system::error_code ec) {
			if(!acceptor_.is_open()) {
				return;
			}

			if(!ec) {
				LOG_DEBUG_FILTER(logger_, LF_NETWORK)
					<< "Accepted connection "
					<< boost::lexical_cast<std::string>(next_connection_->socket().remote_endpoint()) << LOG_ASYNC;

				sessions_.start(next_connection_);
			}

			accept_connection();
		});
	}

public:
	NetworkListener(ServicePool& pool, std::string interface, std::uint16_t port, bool tcp_no_delay, log::Logger* logger)
	                : pool_(pool), logger_(logger), signals_(pool.get_service(), SIGINT, SIGTERM),
	                  acceptor_(pool.get_service(), bai::tcp::endpoint(bai::address::from_string(interface), port)) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(tcp_no_delay));
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		signals_.async_wait(std::bind(&NetworkListener::shutdown, this));
		accept_connection();
	}

	void shutdown() {
		LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;
		acceptor_.close();
	}
};

} // ember