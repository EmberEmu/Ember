/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "NetworkSession.h"
#include "SessionManager.h"
#include <logger/Logger.h>
#include <shared/IPBanCache.h>
#include <shared/memory/ASIOAllocator.h>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <mutex>
#include <utility>
#include <vector>
#include <thread>

namespace ember {

class NetworkListener {
	boost::asio::io_service& service_;
	boost::asio::signal_set signals_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::socket socket_;
	SessionManager sessions_;
	log::Logger* logger_; 
	IPBanCache& ban_list_;
	ASIOAllocator allocator_; // todo - thread_local, VS2015

	void accept_connection() {
		acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
			if(!acceptor_.is_open()) {
				return;
			}

			if(!ec) {
				auto ip = socket_.remote_endpoint().address();

				if(ban_list_.is_banned(ip)) {
					LOG_DEBUG(logger_) << "Rejected connection from banned IP range" << LOG_ASYNC;
					return;
				}

				LOG_DEBUG(logger_) << "Accepted connection " << ip.to_string() << ":"
				                   << socket_.remote_endpoint().port() << LOG_ASYNC;

				start_session(std::move(socket_));
			}

			accept_connection();
		});
	}

	void start_session(boost::asio::ip::tcp::socket socket) {
		LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

		auto& service = socket.get_io_service();
		auto address = socket.remote_endpoint().address().to_string();

		auto session = std::make_shared<Session<T>>(std::move(create_handler_(address)),
		                                            std::move(socket), service);

		session->handler.execute_action =
			std::bind(&NetworkHandler::execute_action, this, session, std::placeholders::_1);
		session->handler.send =
			std::bind(&NetworkHandler::write, this, session, std::placeholders::_1);

		session->timer.expires_from_now(SOCKET_ACTIVITY_TIMEOUT);

		std::lock_guard<std::mutex> guard(sessions_lock_);
		sessions_.insert(session);

		read(session);
	}

	void handle_packet(std::shared_ptr<Session<T>> session) try {
		LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

		if(check_packet_completion_(session->buffer)) {
			if(!session->handler.update_state(session->buffer)) {
				close_session(session);
			}

			session->buffer.clear();
		}

		read(session);
	} catch(std::exception& e) {
		LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
		close_session(session);
	}

public:
	NetworkListener(boost::asio::io_service& service, std::string interface, unsigned short port, bool tcp_no_delay,
	               IPBanCache& bans, ThreadPool& pool, log::Logger* logger, CreateHandler create)
	               : acceptor_(service_, boost::asio::ip::tcp::endpoint(
	                           boost::asio::ip::address::from_string(interface), port)),
	                 socket_(service_), logger_(logger), ban_list_(bans), pool_(pool),
	                 create_handler_(create), signals_(service_, SIGINT, SIGTERM)) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(tcp_no_delay));
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		signals_.async_wait(std::bind(&NetworkListener::shutdown, this));
		accept_connection();
	}

	void shutdown() {
		acceptor_.close();
		sessions_.stop_all();
	}
};

} // ember