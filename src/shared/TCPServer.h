/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logging.h>
#include <boost/asio.hpp>
#include <string>

namespace ember {

template<typename T>
class TCPServer {
	boost::asio::ip::tcp::socket socket_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::io_service& service_;
	log::Logger* logger_;

	void accept_connection() {
		acceptor_.async_accept(socket_,
			[this](boost::system::error_code error) {
				if(!error) {
					LOG_DEBUG(logger_) << "Accepted connection "
					                   << socket_.remote_endpoint().address().to_string()
					                   << ":" << socket_.remote_endpoint().port() << LOG_FLUSH;
					std::make_shared<T>(std::move(socket_), service_)->start();
				}

				accept_connection();
			}
		);
	}

public:
	TCPServer(boost::asio::io_service& service, unsigned short port, log::Logger* logger)
	          : acceptor_(service, bai::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
	            socket_(service), service_(service), logger_(logger) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(true));
		accept_connection();
	}

	TCPServer(boost::asio::io_service& service, unsigned short port, std::string interface, log::Logger* logger)
	          : acceptor_(service, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(interface), port)),
	            service_(service), socket_(service), logger_(logger) {
		accept_connection();
	}
};

} //ember