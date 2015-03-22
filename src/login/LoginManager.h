/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "LoginHandler.h"
#include "PacketBuffer.h"
#include "Session.h"
#include <shared/IPBanCache.h>
#include <shared/memory/ASIOAllocator.h>
#include <boost/asio.hpp>
#include <memory>
#include <string>

namespace ember {

class Action;
class ThreadPool;
namespace log { class Logger; }

class LoginManager {
	typedef std::function<LoginHandler(std::string)> CreateHandler;

	const int SOCKET_ACTIVITY_TIMEOUT = 60;
	const CreateHandler create_handler_;

	boost::asio::ip::tcp::socket socket_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::io_service& service_;
	log::Logger* logger_; 
	IPBanCache<dal::IPBanDAO>& ban_list_;
	ASIOAllocator allocator_; // todo - thread_local, VS2015
	ThreadPool& pool_;

	bool check_packet_completion(const PacketBuffer& buffer);
	bool check_challenge_completion(const PacketBuffer& buffer);
	void handle_packet(std::shared_ptr<Session> session);
	void execute_action(std::shared_ptr<Session>, std::shared_ptr<Action> action);
	void action_complete(std::shared_ptr<Session>, std::shared_ptr<Action> action);

	void accept_connection();
	void start_session(boost::asio::ip::tcp::socket socket);
	void close_session(std::shared_ptr<Session> session);
	void read(std::shared_ptr<Session> session);
	void write(std::shared_ptr<Session> session, std::shared_ptr<Packet> packet);
	void timeout(std::shared_ptr<Session> session, const boost::system::error_code& ec);

public:
	LoginManager(boost::asio::io_service& service, unsigned short port, IPBanCache<dal::IPBanDAO>& bans,
	             ThreadPool& pool, log::Logger* logger, CreateHandler create)
	    	     : acceptor_(service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
	               socket_(service), service_(service), logger_(logger), ban_list_(bans), pool_(pool),
	               create_handler_(create) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(true));
		accept_connection();
	}

	LoginManager(boost::asio::io_service& service, unsigned short port, std::string interface,
	             IPBanCache<dal::IPBanDAO>& bans, ThreadPool& pool, log::Logger* logger, 
				 CreateHandler create)
	             : acceptor_(service,
	               boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(interface), port)),
	               service_(service), socket_(service), logger_(logger), ban_list_(bans), pool_(pool),
	               create_handler_(create) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(true));
		accept_connection();
	}
};

} // ember