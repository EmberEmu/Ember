/*
 * Copyright (c) 2016 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SessionManager.h"
#include "GatewayClient.h"
#include <logger/Logger.h>
#include <boost/asio.hpp>
#include <string>
#include <utility>
#include <cstdint>
#include <cstddef>

#define LF_NETWORK 1 // todo, remove

namespace ember {

namespace bai = boost::asio::ip;

class NetworkListener final {
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::io_service service_;
	boost::asio::ip::tcp::socket socket_;

	SessionManager sessions_;
	log::Logger* logger_;

	void accept_connection() {
		LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

		acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
			if(!acceptor_.is_open()) {
				return;
			}

			if(!ec) {
				const auto ep = socket_.remote_endpoint(ec);

				if(ec) {
					LOG_DEBUG_FILTER(logger_, LF_NETWORK)
						<< "Aborted connection" << LOG_ASYNC;
				} else {
					LOG_DEBUG_FILTER(logger_, LF_NETWORK)
						<< "Accepted connection "
						<< ep.address().to_string()
						<< LOG_ASYNC;

					auto client = std::make_shared<GatewayClient>(
						sessions_, std::move(socket_), ep, logger_
					);

					sessions_.start(std::move(client));
				}
			}

			socket_ = boost::asio::ip::tcp::socket(service_);
			accept_connection();
		});
	}

public:
	NetworkListener(const std::string& interface, std::uint16_t port,
	                bool no_delay, log::Logger* logger)
	                : logger_(logger),
	                  acceptor_(service_, bai::tcp::endpoint(bai::address::from_string(interface), port)),
	                  socket_(service_) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(no_delay));
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