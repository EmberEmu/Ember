/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/Connection.h>
#include <spark/v2/Message.h>
#include <spark/Exception.h>
#include <boost/endian/conversion.hpp>
#include <boost/asio.hpp>
#include <format>
#include <cassert>
#include <cstring>

namespace ba = boost::asio;

namespace ember::spark::v2 {

Connection::Connection(ba::ip::tcp::socket socket, CloseHandler handler)
	: socket_(std::move(socket)),
      strand_(socket_.get_executor()),
	  on_close_(handler) {}

ba::awaitable<void> Connection::process_queue() try {
	while(!queue_.empty()) {
		auto msg = std::move(queue_.front());
		queue_.pop();

		std::array<ba::const_buffer, 2> buffers {
			ba::const_buffer { msg->header.data(), msg->header.size() },
			ba::const_buffer { msg->fbb.GetBufferPointer(), msg->fbb.GetSize() }
		};

		co_await ba::async_write(socket_, buffers, ba::use_awaitable);
	}
} catch(std::exception& e) {
	close();
}

void Connection::send(std::unique_ptr<Message> buffer) {
	ba::post(strand_, [&, buffer = std::move(buffer)]() mutable {
		if(!socket_.is_open()) {
			return;
		}

		bool inactive = queue_.empty();
		queue_.emplace(std::move(buffer));

		if(inactive) {
			ba::co_spawn(strand_, process_queue(), ba::detached);
		}
	});
}

/*
 * This will read until at least the read_size has been received but
 * will read as much as possible into the buffer, with the hope that the
 * entire message will be received with one receive call
 */
ba::awaitable<std::size_t> Connection::read_until(const std::size_t offset,
                                                  const std::size_t read_size) {
	std::size_t received = offset;

	while(received < read_size) {
		auto buffer = ba::buffer(buffer_.data() + received, buffer_.size() - received);
		received += co_await socket_.async_receive(buffer, ba::use_awaitable);
	}

	co_return received;
}

ba::awaitable<std::pair<std::size_t, std::uint32_t>> 
Connection::do_receive(const std::size_t offset) {
	std::size_t rcv_size = offset;
	std::uint32_t msg_size = 0;

	// read at least the message size
	if(rcv_size < sizeof(msg_size)) {
		rcv_size += co_await read_until(offset, sizeof(msg_size));
	}
	
	std::memcpy(&msg_size, buffer_.data(), sizeof(msg_size));
	boost::endian::little_to_native_inplace(msg_size);

	if(msg_size > buffer_.size()) {
		throw std::runtime_error("message too big to fit in receive buffer"); // todo
	}

	// if the entire message wasn't received in a single read, continue reading
	while(rcv_size < msg_size) {
		rcv_size += co_await read_until(rcv_size, msg_size);
	}

	co_return std::make_pair(rcv_size, msg_size);
}


ba::awaitable<void> Connection::begin_receive(ReceiveHandler handler) try {
	std::size_t offset = 0;

	while(socket_.is_open()) {
		auto [rcv_size, msg_size] = co_await do_receive(offset);

		// message complete, handle it
		std::span view(buffer_.data(), msg_size);
		handler(view);

		// move any data belonging to the next message to the start
		if(rcv_size > msg_size) {
			std::memmove(buffer_.data(), buffer_.data() + msg_size, buffer_.size() - msg_size);
		}

		assert(msg_size <= rcv_size);
		offset = rcv_size - msg_size; // buffer offset to start the next read at
	}
} catch(std::exception& e) {
	close();
}

ba::awaitable<std::span<std::uint8_t>> Connection::receive_msg() {
	// read message size uint32
	std::uint32_t msg_size = 0;
	auto buffer = ba::buffer(buffer_.data(), sizeof(msg_size));
	co_await ba::async_read(socket_, buffer, ba::use_awaitable);
	std::memcpy(&msg_size, buffer_.data(), sizeof(msg_size));

	if(msg_size > buffer_.max_size()) {
		throw exception("bad message size");
	}

	// read the rest of the message
	buffer = ba::buffer(buffer_.data() + sizeof(msg_size), msg_size - sizeof(msg_size));
	co_await ba::async_read(socket_, buffer, ba::use_awaitable);
	co_return std::span{buffer_.data(), msg_size};
}

ba::awaitable<void> Connection::send(Message& msg) {
	std::array<ba::const_buffer, 2> buffers {
		ba::const_buffer { msg.header.data(), msg.header.size() },
		ba::const_buffer { msg.fbb.GetBufferPointer(), msg.fbb.GetSize() }
	};

	co_await ba::async_write(socket_, buffers, ba::use_awaitable);
}

// start full-duplex send/receive
void Connection::start(ReceiveHandler handler) {
	ba::co_spawn(strand_, begin_receive(handler), ba::detached);
}

void Connection::close() {
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