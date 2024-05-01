/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "FilterTypes.h"
#include "SessionManager.h"
#include "ClientConnection.h"
#include <logger/Logging.h>
#include <shared/ClientUUID.h>
#include <shared/memory/ASIOAllocator.h>
#include <shared/threading/ServicePool.h>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <utility>
#include <cstdint>
#include <cstddef>

namespace ember {

namespace bai = boost::asio::ip;

class NetworkListener final {
	using tcp_acceptor = boost::asio::basic_socket_acceptor<
		boost::asio::ip::tcp, boost::asio::io_context::executor_type>;

	using tcp_socket = boost::asio::basic_stream_socket<
		boost::asio::ip::tcp, boost::asio::io_context::executor_type>;

	tcp_acceptor acceptor_;

	SessionManager sessions_;
	std::size_t index_;
	ServicePool& pool_;
	log::Logger* logger_;
	tcp_socket socket_;

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
						<< "Aborted connection, remote peer disconnected" << LOG_ASYNC;
				} else {
					LOG_DEBUG_FILTER(logger_, LF_NETWORK)
						<< "Accepted connection " << ep.address().to_string() << LOG_ASYNC;

					auto client = std::make_unique<ClientConnection>(
						sessions_, std::move(socket_), ep,
						ClientUUID::generate(index_), logger_
					);

					sessions_.start(std::move(client));
				}
			}

			++index_;
			index_ %= pool_.size();
			socket_ = tcp_socket(*pool_.get_service(index_));
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