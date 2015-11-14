/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/buffers/ChainedBuffer.h>
#include <spark/SessionManager.h>
#include <logger/Logging.h>
#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <cstdint>

namespace ember { namespace spark {

class NetworkSession : public std::enable_shared_from_this<NetworkSession> {
	const std::chrono::seconds SOCKET_ACTIVITY_TIMEOUT { 60 };

	enum class ReadState { HEADER, BODY };

	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand strand_;
	boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;

	ReadState state_;
	spark::ChainedBuffer<4096> inbound_buffer_;
	SessionManager& sessions_;
	log::Logger* logger_; 
	log::Filter filter_;

	void read() {
		auto self(shared_from_this());
		auto tail = inbound_buffer_.tail();

		// if the buffer chain has no more space left, allocate & attach new node
		if(!tail->free()) {
			tail = inbound_buffer_.allocate();
			inbound_buffer_.attach(tail);
		}

		boost::asio::async_read(socket_, boost::asio::buffer(tail->data(), tail->free()),
			[this, self](boost::system::error_code ec, std::size_t size) {
				if(!ec) {
					set_timer();
					inbound_buffer_.advance_write_cursor(size);

					/*if(handle_packet(inbound_buffer_)) {
						read();
					} else {
						close_session();
					}*/
				} else if(ec != boost::asio::error::operation_aborted) {
					close_session();
				}
			}
		);
	}

	void set_timer() {
		timer_.expires_from_now(SOCKET_ACTIVITY_TIMEOUT);
		timer_.async_wait(strand_.wrap(std::bind(&NetworkSession::timeout, this,
		                                         std::placeholders::_1)));
	}

	void timeout(const boost::system::error_code& ec) {
		if(ec) { // if ec is set, the timer was aborted (session close / refreshed)
			return;
		}

		LOG_DEBUG_FILTER(logger_, filter_)
			<< "Lost connection to Spark peer at " << remote_address() << ":"
			<< remote_port() << " (idle timeout) " << LOG_ASYNC;

		close_session();
	}

	void stop() {
		LOG_DEBUG_FILTER(logger_, filter_)
			<< "Closing Spark connection to " << remote_address()
			<< ":" << remote_port() << LOG_ASYNC;

		boost::system::error_code ec; // we don't care about any errors
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close(ec);
	}

public:
	NetworkSession(SessionManager& sessions, boost::asio::ip::tcp::socket socket,
	               log::Logger* logger, log::Filter filter)
	               : sessions_(sessions), socket_(std::move(socket)), timer_(socket.get_io_service()),
	                 strand_(socket.get_io_service()), logger_(logger), filter_(filter),
	                 state_(ReadState::HEADER) { }

	void start() {
		set_timer();
		read();
	}

	void close_session() {
		sessions_.stop(shared_from_this());
	}

	std::string remote_address() {
		return socket_.remote_endpoint().address().to_string();
	}

	std::uint16_t remote_port() {
		return socket_.remote_endpoint().port();
	}

	template<std::size_t BlockSize>
	void write_chain(std::shared_ptr<spark::ChainedBuffer<BlockSize>> chain) {
		auto self(shared_from_this());

		if(!socket_.is_open()) {
			return;
		}

		socket_.async_send(*chain,
			[this, self, chain](boost::system::error_code ec, std::size_t size) {
				chain->skip(size);

				if(ec && ec != boost::asio::error::operation_aborted) {
					close_session();
				}
			}
		);
	}

	friend class SessionManager;
};

}} // spark, ember