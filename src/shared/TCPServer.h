/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logging.h>
#include <shared/IPBanCache.h>
#include <boost/asio.hpp>
#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <memory>
#include <functional>

namespace ember {

template<typename T>
class TCPServer {
	typedef std::function<std::shared_ptr<T>(boost::asio::ip::tcp::socket)> Create;

	boost::asio::ip::tcp::socket socket_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::io_service& service_;
	log::Logger* logger_; 
	IPBanCache<dal::IPBanDAO>& ban_list_;
	Create create_;

	void accept_connection() {
		acceptor_.async_accept(socket_,
			[this](boost::system::error_code error) {
				if(error) {
					return;
				}
				
				if(!ban_list_.is_banned(socket_.remote_endpoint().address())) {
					LOG_DEBUG(logger_) << "Accepted connection " << socket_.remote_endpoint().address().to_string()
					                   << ":" << socket_.remote_endpoint().port() << LOG_FLUSH;
					create_(std::move(socket_))->start();
					accept_connection();
				} else {
					LOG_DEBUG(logger_) << "Rejected connection from banned IP range" << LOG_FLUSH;
				}
			}
		);
	}

public:
	TCPServer(boost::asio::io_service& service, unsigned short port, IPBanCache<dal::IPBanDAO>& bans,
	          log::Logger* logger, Create create)
	    	  : acceptor_(service, bai::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
	            socket_(service), service_(service), logger_(logger), ban_list_(bans), create_(create) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(true));
		accept_connection();
	}

	TCPServer(boost::asio::io_service& service, unsigned short port, std::string interface,
	          IPBanCache<dal::IPBanDAO>& bans, log::Logger* logger, Create create)
	          : acceptor_(service,
	            boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(interface), port)),
	            service_(service), socket_(service), logger_(logger), create_(create), ban_list_(bans) {
		accept_connection();
	}
};

} //ember