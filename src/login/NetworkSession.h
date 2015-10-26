/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/BufferChain.h>
#include <boost/asio.hpp>
#include <memory>

namespace ember {

class NetworkSession : public std::enable_shared_from_this<NetworkSession> {
	const int SOCKET_ACTIVITY_TIMEOUT = 300;

	spark::BufferChain<1024> inbound_buffer_;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand strand_;
	boost::asio::deadline_timer timer_;

	void read(std::shared_ptr<Session<T>> session) {
		auto& buffer = session->buffer;
		//auto tail = session->buffer_chain_.tail();

		//// if the buffer chain has no more space left, allocate & attach new node
		//if(!tail->free()) {
		//	tail = session->buffer_chain_.allocate();
		//	session->buffer_chain_.attach(tail);
		//}

		session->socket.async_receive(boost::asio::buffer(buffer.store(), buffer.free()),
			session->strand.wrap(create_alloc_handler(allocator_,
			[this, session](boost::system::error_code ec, std::size_t size) {
				if(!ec) {
					reset_timer(session);
					session->buffer.advance(size);
					handle_packet(session);
				} else if(ec != boost::asio::error::operation_aborted) {
					close_session(session);
				}
			}
		)));
	}

public:
	void start() {

	}

	typedef std::shared_ptr<NetworkSession> SharedNetSession;
};

} // ember