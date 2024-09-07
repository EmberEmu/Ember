/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SessionManager.h"
#include "FilterTypes.h"
#include <logger/Logger.h>
#include <spark/buffers/pmr/BinaryStream.h>
#include <spark/buffers/DynamicBuffer.h>
#include <spark/buffers/BufferSequence.h>
#include <shared/memory/ASIOAllocator.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <cstdint>

namespace ember {

template<typename T>
class NetworkSession : public std::enable_shared_from_this<NetworkSession<T>> {
public:
	using WriteCallback = std::function<void()>;

private:
	using Buffer = spark::io::DynamicBuffer<1024>;

	const std::chrono::seconds SOCKET_ACTIVITY_TIMEOUT { 60 };
	ASIOAllocator<thread_safe> allocator_;

	SessionManager& sessions_;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::steady_timer timer_;

	Buffer inbound_buffer_;
	Buffer* outbound_front_;
	Buffer* outbound_back_;
	std::array<Buffer, 2> outbound_buffers_{};
	bool write_in_progress_;

	log::Logger* logger_;
	bool stopped_;

	void read() {
		auto self(this->shared_from_this());
		auto tail = inbound_buffer_.back();

		// if the buffer chain has no more space left, allocate & attach new node
		if(!tail || !tail->free()) {
			tail = inbound_buffer_.allocate();
			inbound_buffer_.push_back(tail);
		}

		set_timer();

		socket_.async_receive(boost::asio::buffer(tail->write_data(), tail->free()), 
			create_alloc_handler(allocator_,
			[this, self](boost::system::error_code ec, std::size_t size) {
				if(stopped_) {
					return;
				}

				timer_.cancel();

				if(!ec) {
					inbound_buffer_.advance_write(size);

					if(static_cast<T*>(this)->handle_packet(inbound_buffer_)) {
						read();
					} else {
						close_session();
					}
				} else if(ec != boost::asio::error::operation_aborted) {
					close_session();
				}
			}
		));
	}

	void write(WriteCallback cb) {
		auto self(this->shared_from_this());
		set_timer();

		const spark::io::BufferSequence sequence(*outbound_front_);

		socket_.async_send(sequence, create_alloc_handler(allocator_,
			[this, self, cb = std::move(cb)](boost::system::error_code ec, std::size_t size) mutable {
			outbound_front_->skip(size);

			if(!ec) {
				if(!outbound_front_->empty()) {
					write(std::move(cb)); // entire buffer wasn't sent, hit gather-write limits?
				} else {
					std::swap(outbound_front_, outbound_back_);

					if(!outbound_front_->empty()) {
						write(std::move(cb));
					} else { // all done!
						write_in_progress_ = false;
					
						if(cb) {
							cb();
						}
					}
				}
			} else if(ec != boost::asio::error::operation_aborted) {
				close_session();
			}
		}));
	}

	void set_timer() {
		auto self(this->shared_from_this());

		timer_.expires_from_now(SOCKET_ACTIVITY_TIMEOUT);
		timer_.async_wait([this, self](const boost::system::error_code& ec) {
			timeout(ec);
		});
	}

	void timeout(const boost::system::error_code& ec) {
		if(ec || stopped_) { // if ec is set, the timer was aborted (session close / refreshed)
			return;
		}

		LOG_DEBUG_FILTER(logger_, LF_NETWORK)
			<< "Idle timeout triggered on " << remote_address() << LOG_ASYNC;

		close_session();
	}

public:
	NetworkSession(SessionManager& sessions, boost::asio::ip::tcp::socket socket, log::Logger* logger)
	               : sessions_(sessions),
	                 socket_(std::move(socket)),
	                 timer_(socket_.get_executor()),
	                 outbound_front_(&outbound_buffers_.front()),
	                 outbound_back_(&outbound_buffers_.back()),
	                 write_in_progress_(false),
	                 logger_(logger),
	                 stopped_(false) { }

	void start() {
		read();
	}

	std::string remote_address() const {
		return socket_.remote_endpoint().address().to_string();
	}

	void close_session() {
		sessions_.stop(this->shared_from_this());
	}

	void write(const auto& data, WriteCallback cb) {
		if(!socket_.is_open()) {
			return;
		}

		spark::io::pmr::BinaryStream stream(*outbound_back_);
		data.write_to_stream(stream); // todo, provide operator<< for packets?

		if(!write_in_progress_) {
			write_in_progress_ = true;
			std::swap(outbound_front_, outbound_back_);
			write(std::move(cb));
		} else {
			cb();
		}
	}

	boost::asio::any_io_executor get_executor() {
		return socket_.get_executor();
	}

	void stop() {
		auto self(this->shared_from_this());

		boost::asio::post(socket_.get_executor(), [this, self] {
			LOG_DEBUG_FILTER(logger_, LF_NETWORK)
				<< "Closing connection to " << remote_address() << LOG_ASYNC;

			stopped_ = true;
			boost::system::error_code ec; // we don't care about any errors
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			socket_.close(ec);
			timer_.cancel();
		});
	}

	virtual ~NetworkSession() = default;
};

} // ember