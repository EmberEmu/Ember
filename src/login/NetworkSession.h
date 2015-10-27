/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "PacketHandler.h"
#include "SessionManager.h"
#include <logger/Logging.h>
#include <spark/BufferChain.h>
#include <shared/memory/ASIOAllocator.h>
#include <shared/threading/ThreadPool.h>
#include <boost/asio.hpp>
#include <chrono>
#include <memory>

namespace ember {

class NetworkSession : public std::enable_shared_from_this<NetworkSession> {
	const std::chrono::seconds SOCKET_ACTIVITY_TIMEOUT { 300 };

	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand strand_;
	boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;

	spark::BufferChain<1024> inbound_buffer_;
	SessionManager& sessions_;
	ASIOAllocator allocator_; // temp - should be passed in
	log::Logger* logger_; 

	void read() {
		auto self(shared_from_this());
		auto tail = inbound_buffer_.tail();

		// if the buffer chain has no more space left, allocate & attach new node
		if(!tail->free()) {
			tail = inbound_buffer_.allocate();
			inbound_buffer_.attach(tail);
		}

		socket_.async_receive(boost::asio::buffer(tail->data(), tail->free()),
			strand_.wrap(create_alloc_handler(allocator_,
			[this, self](boost::system::error_code ec, std::size_t size) {
				if(!ec) {
					set_timer();
					inbound_buffer_.advance_write_cursor(size);
					handle_packet(inbound_buffer_);
					read();
				} else if(ec != boost::asio::error::operation_aborted) {
					sessions_.stop(shared_from_this());
				}
			}
		)));
	}

	void set_timer() {
		timer_.expires_from_now(SOCKET_ACTIVITY_TIMEOUT);
		timer_.async_wait(strand_.wrap(std::bind(&NetworkSession::timeout, this,
		                                         std::placeholders::_1)));
	}

	void timeout(const boost::system::error_code& ec) {
		if(!ec) { // if the connection timed out, otherwise timer was aborted (session close)
			stop();
		}
	}

	//void action_complete(std::shared_ptr<NetworkSession> session, std::shared_ptr<Action> action) try {
	//	/*if(!session->handler.update_state(action)) {
	//		sessions_.stop(session);
	//	}*/
	//} catch(std::exception& e) {
	//	LOG_DEBUG(logger_) << e.what() << LOG_ASYNC;
	//	sessions_.stop(session);
	//}

public:
	NetworkSession(SessionManager& sessions, boost::asio::ip::tcp::socket socket, log::Logger* logger)
	: sessions_(sessions), socket_(std::move(socket)), timer_(socket.get_io_service()),
	  strand_(socket.get_io_service()), logger_(logger) { }

	void start() {
		read();
	}

	void stop() {
		boost::system::error_code ec; // we don't care about any errors
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close();
	}

	virtual void handle_packet(spark::Buffer& buffer) = 0;
	//void execute_action(std::shared_ptr<Action> action) {
	//	auto self(shared_from_this());

	//	pool_.run([action, this, self] {
	//		action->execute();
	//		strand_.post([action, this, self] {
	//			action_complete(self, action);
	//		});
	//	});
	//}

	void write(std::shared_ptr<char> packet) {
		//session->tbuffer.write(packet->data(), packet->size());
		//LOG_WARN(logger_) << "Packet size: " << packet->size() << LOG_SYNC;
		//LOG_WARN(logger_) << "Chain size: " << session->tbuffer.size() << LOG_SYNC;
		//session->socket.async_send(session->tbuffer,
		//	session->strand.wrap(create_alloc_handler(allocator_,
		//	[this, packet, session](boost::system::error_code ec, std::size_t size) {
		//LOG_WARN(logger_) << "Send size: " << size << LOG_SYNC;

		//		if(!ec) {
		//			session->tbuffer.skip(size);

		//			if(session->tbuffer.size()) {
		//				// do additional write
		//			}
		//		} else if(ec && ec != boost::asio::error::operation_aborted) {
		//			close_session(session);
		//		}
		//	}
		//)));
	}

	virtual ~NetworkSession() = default;
};

} // ember