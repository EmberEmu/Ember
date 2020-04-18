/*
 * Copyright (c) 2015 - 2020 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "NetworkSession.h"
#include "SessionBuilders.h"
#include "SessionManager.h"
#include "FilterTypes.h"
#include <logger/Logger.h>
#include <shared/IPBanCache.h>
#include <shared/memory/ASIOAllocator.h>
#include <shared/metrics/Metrics.h>
#include <boost/asio.hpp>
#include <string>
#include <utility>
#include <cstdint>
#include <cstddef>

namespace ember {

class NetworkListener {
	boost::asio::io_context& service_;
	boost::asio::signal_set signals_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::socket socket_;

	const NetworkSessionBuilder& session_create_;
	SessionManager sessions_;
	log::Logger* logger_;
	Metrics& metrics_;
	IPBanCache& ban_list_;
	ASIOAllocator allocator_; // todo - thread_local, VS2015

	void accept_connection() {
		LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

		acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
			if(!acceptor_.is_open()) {
				return;
			}

			if(!ec) {
				const auto ep = socket_.remote_endpoint(ec);
				const auto ip = ep.address();

				if(ec) {
					LOG_DEBUG_FILTER(logger_, LF_NETWORK)
						<< "Aborted connection, remote peer disconnected" << LOG_ASYNC;
					metrics_.increment("aborted_connections");
				} else if(ban_list_.is_banned(ip)) {
					LOG_DEBUG_FILTER(logger_, LF_NETWORK)
						<< "Rejected connection " << ip.to_string()
						<< " from banned IP range" << LOG_ASYNC;
					metrics_.increment("rejected_connections");
				} else {
					LOG_DEBUG_FILTER(logger_, LF_NETWORK)
						<< "Accepted connection " << ip.to_string() << LOG_ASYNC;
					metrics_.increment("accepted_connections");
					start_session(std::move(socket_), ep);
				}
			}

			accept_connection();
		});
	}

	void start_session(boost::asio::ip::tcp::socket socket, boost::asio::ip::tcp::endpoint ep) {
		LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;
		auto session = session_create_.create(sessions_, std::move(socket), ep, logger_);
		sessions_.start(session);
	}

public:
	NetworkListener(boost::asio::io_context& service, const std::string& interface, std::uint16_t port,
	                bool tcp_no_delay, const NetworkSessionBuilder& session_create, IPBanCache& bans,
	                log::Logger* logger, Metrics& metrics)
	                : acceptor_(service_, boost::asio::ip::tcp::endpoint(
	                            boost::asio::ip::address::from_string(interface), port)),
	                  service_(service), socket_(service_), logger_(logger), ban_list_(bans),
	                  signals_(service_, SIGINT, SIGTERM), session_create_(session_create),
	                  metrics_(metrics) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(tcp_no_delay));
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		signals_.async_wait([this](auto& error, auto signal) { shutdown(); });
		accept_connection();
	}

	void shutdown() {
		LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;
		acceptor_.close();
		sessions_.stop_all();
	}

	std::size_t connection_count() const {
		return sessions_.count();
	}
};

} // ember