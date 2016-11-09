/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SocketHandler.h"
#include <logger/Logging.h>
#include <boost/asio.hpp>
#include <string>
#include <utility>
#include <cstdint>

#define LF_NETWORK 1 // todo, remove

namespace ember {

class Listener {
	boost::asio::io_service service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	bool stop_;

	log::Logger* logger_;

	void accept_connection() {
		LOG_TRACE_FILTER(logger_, LF_NETWORK) << __func__ << LOG_ASYNC;

		boost::system::error_code ec;
		boost::asio::ip::tcp::socket socket_(service_);

		while(!stop_) {
			acceptor_.accept(socket_, ec);

			if(ec) {
				LOG_ERROR_FILTER(logger_, LF_NETWORK) << ec.message() << LOG_ASYNC;
				continue;
			}

		}
	}

public:
	Listener(const std::string& interface, std::uint16_t port, bool no_delay,
			 log::Logger* logger)
	         : acceptor_(service_), stop_(false), logger_(logger) {
		acceptor_.set_option(boost::asio::ip::tcp::no_delay(no_delay));
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

		service_.post([this]{
			accept_connection();
		});
	}

	void shutdown() {
		 
	}
};

} // ember