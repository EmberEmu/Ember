/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Actions.h"
#include "PacketBuffer.h"
#include "Session.h"
#include <logger/Logger.h>
#include <shared/IPBanCache.h>
#include <shared/memory/ASIOAllocator.h>
#include <shared/threading/ThreadPool.h>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace ember {

template<typename T>
class NetworkHandler {
	typedef std::function<T(std::string)> CreateHandler;
	typedef std::function<bool(const PacketBuffer&)> CompletionChecker;

	const int SOCKET_ACTIVITY_TIMEOUT = 300;
	const CreateHandler create_handler_;
	const CompletionChecker check_packet_completion_;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::io_service& service_;
	log::Logger* logger_; 
	IPBanCache<dal::IPBanDAO>& ban_list_;
	ASIOAllocator allocator_; // todo - thread_local, VS2015
	ThreadPool& pool_;

	void accept_connection() {
		acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
			if(ec) {
				return;
			}

			auto ip = socket_.remote_endpoint().address();

			if(ban_list_.is_banned(ip)) {
				LOG_DEBUG(logger_) << "Rejected connection from banned IP range" << LOG_ASYNC;
				return;
			}

			LOG_DEBUG(logger_) << "Accepted connection " << ip.to_string() << ":"
							   << socket_.remote_endpoint().port() << LOG_ASYNC;

			start_session(std::move(socket_));
			accept_connection();
		});
	}

	void start_session(boost::asio::ip::tcp::socket socket) {
		LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

		auto& service = socket.get_io_service();
		auto address = socket.remote_endpoint().address().to_string();

		auto session = std::make_shared<Session<T>>(std::move(create_handler_(address)),
		                                            std::move(socket), service);

		session->handler.on_action =
			std::bind(&NetworkHandler::execute_action, this, session, std::placeholders::_1);
		session->handler.on_send =
			std::bind(&NetworkHandler::write, this, session, std::placeholders::_1);

		session->timer.expires_from_now(boost::posix_time::seconds(SOCKET_ACTIVITY_TIMEOUT));
		read(session);
	}

	void read(std::shared_ptr<Session<T>> session) {
		reset_timer(session);
		auto& buffer = session->buffer;

		session->socket.async_receive(boost::asio::buffer(buffer.store(), buffer.free()),
			session->strand.wrap(create_alloc_handler(allocator_,
			[this, session](boost::system::error_code ec, std::size_t size) {
				if(!ec) {
					reset_timer(session);
					session->buffer.advance(size);
					handle_packet(session);
				} else {
					session->timer.cancel();
				}
			}
		)));
	}

	void write(std::shared_ptr<Session<T>> session, std::shared_ptr<Packet> packet) {
		session->socket.async_send(boost::asio::buffer(*packet),
			session->strand.wrap(create_alloc_handler(allocator_,
			[this, packet, session](boost::system::error_code ec, std::size_t) {
				if(!ec) {
					reset_timer(session);
				} else {
					session->timer.cancel();
				}
			}
		)));
	}

	void action_complete(std::shared_ptr<Session<T>> session, std::shared_ptr<Action> action) try {
		if(!session->handler.update_state(action)) {
			close_session(session);
		}
	} catch(std::exception& e) {
		LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
		close_session(session);
	}

	void execute_action(std::shared_ptr<Session<T>> session, std::shared_ptr<Action> action) {
		pool_.run([session, action, this]() {
			action->execute();
			session->strand.post(std::bind(&NetworkHandler::action_complete, this, session, action));
		});
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

	void close_session(std::shared_ptr<Session<T>> session) {
		boost::system::error_code ec;
		session->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		session->socket.close();
	}

	void reset_timer(std::shared_ptr<Session<T>> session) {
		session->timer.expires_from_now(boost::posix_time::seconds(SOCKET_ACTIVITY_TIMEOUT));
		session->timer.async_wait(session->strand.wrap(std::bind(&NetworkHandler::timeout, this,
		                          session, std::placeholders::_1)));
	}

	void timeout(std::shared_ptr<Session<T>> session, const boost::system::error_code& ec) {
		LOG_TRACE(logger_) << __func__ << LOG_SYNC;

		if(!ec) {
			LOG_TRACE(logger_) << "Yes" << LOG_SYNC;
			close_session(session);
		}
	}

public:
	NetworkHandler(boost::asio::io_service& service, unsigned short port, IPBanCache<dal::IPBanDAO>& bans,
	               ThreadPool& pool, log::Logger* logger, CreateHandler create, CompletionChecker checker)
	    	       : acceptor_(service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
	                socket_(service), service_(service), logger_(logger), ban_list_(bans), pool_(pool),
	                create_handler_(create), check_packet_completion_(checker) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(true));
		accept_connection();
	}

	NetworkHandler(boost::asio::io_service& service, unsigned short port, std::string interface,
	               IPBanCache<dal::IPBanDAO>& bans, ThreadPool& pool, log::Logger* logger, 
	               CreateHandler create, CompletionChecker checker)
	               : acceptor_(service,
	                boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(interface), port)),
	                service_(service), socket_(service), logger_(logger), ban_list_(bans), pool_(pool),
	                create_handler_(create), check_packet_completion_(checker) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(true));
		accept_connection();
	}
};

} // ember