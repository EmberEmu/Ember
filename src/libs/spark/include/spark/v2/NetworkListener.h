/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/v2/SocketAcceptor.h>
#include <logger/Logging.h>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <utility>
#include <cstdint>
#include <cstddef>

namespace ember::spark::v2 {

namespace bai = boost::asio::ip;

class NetworkListener {
	SocketAcceptor& sock_acc_;
	boost::asio::io_context& context_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::socket socket_;
	log::Logger* logger_;

	void accept_connection() {
		LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

		acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
			if(!acceptor_.is_open()) {
				return;
			}

			if(!ec) {
				const auto ep = socket_.remote_endpoint(ec);

				if(ec) {
					LOG_DEBUG(logger_)
						<< "[spark] Aborted connection, remote peer disconnected" << LOG_ASYNC;
				} else {
					LOG_DEBUG(logger_)
						<< "[spark] Accepted connection " << ep.address().to_string()
						<< ":" << ep.port() << LOG_ASYNC;

					sock_acc_.accept(std::move(socket_));
				}
			}

			socket_ = boost::asio::ip::tcp::socket(context_);
			accept_connection();
		});
	}

public:
	NetworkListener(SocketAcceptor& sock_acc, boost::asio::io_context& context,
	                const std::string& interface, std::uint16_t port,
	                bool tcp_no_delay, log::Logger* logger)
	                : sock_acc_(sock_acc), context_(context), logger_(logger), acceptor_(context_,
	                  bai::tcp::endpoint(bai::address::from_string(interface), port)),
	                  socket_(context_) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(tcp_no_delay));
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		accept_connection();
	}

	void shutdown() {
		LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
		acceptor_.close();
	}
};

} // spark, ember