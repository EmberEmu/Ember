/*
 * Copyright (c) 2015, 2016 Ember
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
#include <shared/ClientUUID.h>
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
	boost::asio::ip::tcp::acceptor acceptor_;

	SessionManager sessions_;
	std::size_t index_;
	ServicePool& pool_;
	log::Logger* logger_;
	boost::asio::ip::tcp::socket socket_;

	void accept_connection() {
		LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

		acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
			if(!acceptor_.is_open()) {
				return;
			}

			if(!ec) {
				LOG_DEBUG_FILTER(logger_, LF_NETWORK)
					<< "Accepted connection "
					<< boost::lexical_cast<std::string>(socket_.remote_endpoint())
					<< LOG_ASYNC;

				auto client = std::make_unique<ClientConnection>(
					sessions_, std::move(socket_),
					ClientUUID::generate(index_), logger_
				);

				sessions_.start(std::move(client));
			}

			++index_ %= pool_.size();
			socket_ = boost::asio::ip::tcp::socket(*pool_.get_service(index_));
			accept_connection();
		});
	}

public:
	NetworkListener(ServicePool& pool, const std::string& interface, std::uint16_t port,
	                bool tcp_no_delay, log::Logger* logger)
	                : pool_(pool), logger_(logger),
	                  index_(0), acceptor_(pool.get_service(),
	                  bai::tcp::endpoint(bai::address::from_string(interface), port)),
	                  socket_(*pool.get_service(0)) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(tcp_no_delay));
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		accept_connection();
	}

	void shutdown() {
		LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;
		acceptor_.close();
		sessions_.stop_all();
	}
};

} // ember