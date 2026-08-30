/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/Connection.h>
#include <spark/Exception.h>
#include <logger/Logger.h>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/endian/conversion.hpp>
#include <format>
#include <cassert>
#include <cstring>

namespace asio = boost::asio;

namespace ember::spark {

Connection::Connection(asio::ip::tcp::socket socket, log::Logger& logger, CloseHandler handler)
	: logger_(logger),
	  socket_(std::move(socket)),
      strand_(socket_.get_executor()),
	  on_close_(handler) {
	buffer_.resize(4); // todo
}

asio::awaitable<void> Connection::process_queue() try {
	while(!queue_.empty()) {
		const auto msg = std::move(queue_.front());
		queue_.pop();

		std::array buffers {
			asio::const_buffer { msg.header.data(), msg.header.size() },
			asio::const_buffer { msg.fbb.GetBufferPointer(), msg.fbb.GetSize() }
		};

		co_await asio::async_write(socket_, buffers);
	}
} catch(const std::exception&) {
	close();
}

void Connection::send(Message&& buffer) {
	asio::post(strand_, [&, buffer = std::move(buffer)]() mutable {
		if(!socket_.is_open()) {
			return;
		}

		const bool inactive = queue_.empty();
		queue_.emplace(std::move(buffer));

		if(inactive) {
			asio::co_spawn(strand_, process_queue(), asio::detached);
		}
	});
}

/*
 * This will read until at least the read_size has been received but
 * will read as much as possible into the buffer, with the hope that the
 * entire message will be received with one receive call
 */
asio::awaitable<std::size_t> Connection::read_until(const std::size_t offset,
                                                    const std::size_t read_size) {
	std::size_t received = offset;

	while(received < read_size) {
		auto buffer = asio::buffer(buffer_.data() + received, buffer_.size() - received);
		received += co_await socket_.async_receive(buffer);
	}

	co_return received;
}

void Connection::buffer_resize(const std::uint32_t size) {
	LOG_TRACE(logger_, log_func);

	if(size > MAXIMUM_BUFFER_SIZE) {
		const auto log_msg = std::format(
			"maximum message ({}b) exceeded", MAXIMUM_BUFFER_SIZE
		);

		throw exception(log_msg);
	}

	LOG_TRACE(logger_, "Resizing RPC buffer to {}b", size);
	buffer_.resize(size);
}

asio::awaitable<std::uint32_t>  Connection::do_receive() {
	std::uint32_t msg_size = 0;

	// read the message size
	auto buf = boost::asio::buffer(buffer_.data(), sizeof(msg_size));
	co_await socket_.async_read_some(buf);
	
	std::memcpy(&msg_size, buffer_.data(), sizeof(msg_size));
	boost::endian::little_to_native_inplace(msg_size);

	if(msg_size > buffer_.size()) {
		buffer_resize(msg_size);
	}

	buf = boost::asio::buffer(buffer_.data() + sizeof(msg_size), msg_size - sizeof(msg_size));
	co_await socket_.async_read_some(buf);
	co_return msg_size;
}

asio::awaitable<void> Connection::begin_receive(ReceiveHandler handler) try {
	while(socket_.is_open()) {
		const auto msg_size = co_await do_receive();

		// message complete, handle it
		std::span view(buffer_.data(), msg_size);
		handler(view);
	}
} catch(const std::exception& e) {
	LOG_WARN(logger_, e.what());
	close();
}

asio::awaitable<std::span<std::uint8_t>> Connection::receive_msg() {
	// read message size uint32
	std::uint32_t msg_size = 0;

	auto buffer = asio::buffer(buffer_.data(), sizeof(msg_size));
	co_await asio::async_read(socket_, buffer);
	std::memcpy(&msg_size, buffer_.data(), sizeof(msg_size));

	if(msg_size > buffer_.size()) {
		buffer_resize(msg_size);
	}

	// read the rest of the message
	buffer = asio::buffer(buffer_.data() + sizeof(msg_size), msg_size - sizeof(msg_size));
	co_await asio::async_read(socket_, buffer);
	co_return std::span{buffer_.data(), msg_size};
}

asio::awaitable<void> Connection::send(Message& msg) {
	std::array buffers {
		asio::const_buffer { msg.header.data(), msg.header.size() },
		asio::const_buffer { msg.fbb.GetBufferPointer(), msg.fbb.GetSize() }
	};

	co_await asio::async_write(socket_, buffers);
}

// start full-duplex send/receive
void Connection::start(ReceiveHandler handler) {
	LOG_TRACE(logger_, log_func);
	asio::co_spawn(strand_, begin_receive(handler), asio::detached);
}

void Connection::close() {
	LOG_TRACE(logger_, log_func);

	socket_.close();

	if(on_close_) {
		on_close_();
	}
}

std::string Connection::address() const {
	if(!socket_.is_open()) {
		return "";
	}

	const auto& ep = socket_.remote_endpoint();
	return std::format("{}:{}", ep.address().to_string(), ep.port());
}

} // spark, ember