/*
 * Copyright (c) 2015 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SessionManager.h"
#include "FilterTypes.h"
#include <logger/Logging.h>
#include <spark/buffers/ChainedBuffer.h>
#include <spark/buffers/BufferSequence.h>
#include <shared/memory/ASIOAllocator.h>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <cstdint>

namespace ember {

class NetworkSession : public std::enable_shared_from_this<NetworkSession> {
	const std::chrono::seconds SOCKET_ACTIVITY_TIMEOUT { 60 };

	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand strand_;
	boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;

	spark::ChainedBuffer<1024> inbound_buffer_;
	SessionManager& sessions_;
	ASIOAllocator allocator_; // temp - should be passed in
	const std::string remote_address_;
	log::Logger* logger_;
	bool stopped_;

	void read() {
		auto self(shared_from_this());
		auto tail = inbound_buffer_.back();

		// if the buffer chain has no more space left, allocate & attach new node
		if(!tail->free()) {
			tail = inbound_buffer_.allocate();
			inbound_buffer_.push_back(tail);
		}

		set_timer();

		socket_.async_receive(boost::asio::buffer(tail->write_data(), tail->free()),
			strand_.wrap(create_alloc_handler(allocator_,
			[this, self](boost::system::error_code ec, std::size_t size) {
				if(stopped_) {
					return;
				}

				timer_.cancel();

				if(!ec) {
					inbound_buffer_.advance_write_cursor(size);

					if(handle_packet(inbound_buffer_)) {
						read();
					} else {
						close_session();
					}
				} else if(ec != boost::asio::error::operation_aborted) {
					close_session();
				}
			}
		)));
	}

	void set_timer() {
		auto self(shared_from_this());

		timer_.expires_from_now(SOCKET_ACTIVITY_TIMEOUT);
		timer_.async_wait(strand_.wrap(
			[this, self](const boost::system::error_code& ec) {
				timeout(ec);
			}
		));
	}

	void timeout(const boost::system::error_code& ec) {
		if(ec || stopped_) { // if ec is set, the timer was aborted (session close / refreshed)
			return;
		}

		LOG_DEBUG_FILTER(logger_, LF_NETWORK)
			<< "Idle timeout triggered on " << remote_address() << LOG_ASYNC;

		close_session();
	}

	void stop() {
		auto self(shared_from_this());

		strand_.post([this, self] {
			LOG_DEBUG_FILTER(logger_, LF_NETWORK)
				<< "Closing connection to " << remote_address() << LOG_ASYNC;

			stopped_ = true;
			boost::system::error_code ec; // we don't care about any errors
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			socket_.close(ec);
			timer_.cancel();
		});
	}

public:
	NetworkSession(SessionManager& sessions, boost::asio::ip::tcp::socket socket, log::Logger* logger)
	               : sessions_(sessions), socket_(std::move(socket)), timer_(socket.get_io_service()),
	                 strand_(socket.get_io_service()), logger_(logger), stopped_(false),
	                 remote_address_(boost::lexical_cast<std::string>(socket_.remote_endpoint())) { }

	virtual void start() {
		read();
	}

	std::string remote_address() {
		return remote_address_;
	}

	std::uint16_t remote_port() {
		return socket_.remote_endpoint().port();
	}

	virtual void close_session() {
		sessions_.stop(shared_from_this());
	}

	template<std::size_t BlockSize>
	void write_chain(const std::shared_ptr<spark::ChainedBuffer<BlockSize>>& chain, bool notify) {
		auto self(shared_from_this());

		if(!socket_.is_open()) {
			return;
		}

		set_timer();

		spark::BufferSequence<BlockSize> sequence(*chain);

		socket_.async_send(sequence,
			strand_.wrap(create_alloc_handler(allocator_,
			[=](boost::system::error_code ec, std::size_t size) {
				chain->skip(size);

				if(ec && ec != boost::asio::error::operation_aborted) {
					close_session();
				} else if(!ec && chain->size()) {
					write_chain(chain, notify); 
				} else if(notify) {
					on_write_complete();
				}
			}
		)));
	}

	boost::asio::strand& strand() { return strand_;  }

	virtual bool handle_packet(spark::Buffer& buffer) = 0;
	virtual void on_write_complete() = 0;
	virtual ~NetworkSession() = default;

	friend class SessionManager;
};

} // ember