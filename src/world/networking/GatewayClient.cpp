/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "GatewayClient.h"
#include "SessionManager.h"

namespace ember {

#define LF_NETWORK 1 // todo, remove

void GatewayClient::write() {

}

void GatewayClient::read() {

}

std::string GatewayClient::remote_address() {
	return address_;
}

void GatewayClient::start() {
	stopped_ = false;
	//handler_.start();
	read();
}

void GatewayClient::stop() {
	LOG_DEBUG_FILTER(logger_, LF_NETWORK)
		<< "Closing connection to " << remote_address() << LOG_ASYNC;

	//handler_.stop();
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	socket_.close(ec);
	stopped_ = true;
}

/* 
 * This function should only be used by the handler to allow for the session to be
 * queued for closure after it has returned from processing the current event/packet.
 * Posting rather than dispatching ensures that the object won't be destroyed by the
 * session manager until current processing has finished.
 */
void GatewayClient::close_session() {
	socket_.get_io_service().post([this] {
		sessions_.stop(this);
	});
}

/*
 * This function is used by the destructor to ensure that all current processing
 * has finished before it returns. It uses dispatch rather than post to ensure
 * that if the calling thread happens to be the owner of this connection, that
 * it will be closed immediately, 'in line', rather than blocking indefinitely.
 */
void GatewayClient::close_session_sync() {
	socket_.get_io_service().dispatch([&] {
		stop();

		std::unique_lock<std::mutex> ul(stop_lock_);
		stop_condvar_.notify_all();
	});
}

void GatewayClient::terminate() {
	if(!stopped_) {
		close_session_sync();

		while(!stopped_) {
			std::unique_lock<std::mutex> guard(stop_lock_);
			stop_condvar_.wait(guard);
		}
	}
}

/*
 * Closes the socket and then posts a final event that keeps the client alive
 * until all pending handlers are executed with 'operation_aborted'.
 * That's the theory anyway.
 */
void GatewayClient::async_shutdown(std::shared_ptr<GatewayClient> client) {
	client->terminate();

	client->socket_.get_io_service().post([client]() {
		LOG_TRACE_FILTER_GLOB(LF_NETWORK) << "Handler for " << client->remote_address()
			<< " destroyed" << LOG_ASYNC;
	});
}

} // ember