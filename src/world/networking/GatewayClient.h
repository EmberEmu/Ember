/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/Logging.h>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <utility>

namespace ember {

class SessionManager;

class GatewayClient final {
	boost::asio::ip::tcp::socket socket_;

	SessionManager& sessions_;
	log::Logger* logger_;
	const std::string address_;

	std::condition_variable stop_condvar_;
	std::mutex stop_lock_;
	std::atomic_bool stopped_;

	// socket I/O
	void read();
	void write();

	// session management
	void stop();
	void close_session_sync();

public:
	GatewayClient(SessionManager& sessions, boost::asio::ip::tcp::socket socket,
				  log::Logger* logger)
	              : sessions_(sessions), logger_(logger), socket_(std::move(socket)),
	                address_(boost::lexical_cast<std::string>(socket_.remote_endpoint())) { }

	void start();
	void close_session();
	void terminate();
	std::string remote_address();

	static void async_shutdown(std::shared_ptr<GatewayClient> client);
};

} // ember