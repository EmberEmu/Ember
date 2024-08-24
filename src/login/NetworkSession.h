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
#include <logger/Logging.h>
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

class NetworkSession : public std::enable_shared_from_this<NetworkSession> {
public:
	using WriteCallback = std::function<void()>;

private:
	using Buffer = spark::io::DynamicBuffer<1024>;

	const std::chrono::seconds SOCKET_ACTIVITY_TIMEOUT { 60 };
	ASIOAllocator<thread_safe> allocator_;

	boost::asio::ip::tcp::socket socket_;
	const boost::asio::ip::tcp::endpoint remote_ep_;
	boost::asio::steady_timer timer_;

	Buffer inbound_buffer_;
	Buffer* outbound_front_;
	Buffer* outbound_back_;
	std::array<Buffer, 2> outbound_buffers_{};
	bool write_in_progress_;

	SessionManager& sessions_;
	log::Logger* logger_;
	bool stopped_;

	void read();
	void write(WriteCallback cb);
	void set_timer();
	void timeout(const boost::system::error_code& ec);

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
		sessions_.stop(shared_from_this());
	}

	template<typename StreamSerialisable>
	void write(const StreamSerialisable& data, WriteCallback cb) {
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

	void stop();
	virtual bool handle_packet(spark::io::pmr::Buffer& buffer) = 0;
	virtual ~NetworkSession() = default;
};

} // ember