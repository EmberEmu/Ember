/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/PeerConnection.h>
#include <spark/v2/Message.h>
#include <boost/endian/conversion.hpp>
#include <boost/asio.hpp>
#include <array>
#include <format>
#include <span>
#include <cassert>
#include <cstring>

namespace ba = boost::asio;

namespace ember::spark::v2 {

PeerConnection::PeerConnection(Dispatcher& dispatcher, ba::ip::tcp::socket socket)
	: dispatcher_(dispatcher),
	  socket_(std::move(socket)),
      strand_(socket_.get_executor()) {
	ba::co_spawn(strand_, receive(), ba::detached);
}

ba::awaitable<void> PeerConnection::process_queue() try {
	while(!queue_.empty()) {
		auto msg = std::move(queue_.front());
		queue_.pop();

		std::array<boost::asio::const_buffer, 2> buffers {
			boost::asio::const_buffer { msg->header.data(), msg->header.size() },
			boost::asio::const_buffer { msg->fbb.GetBufferPointer(), msg->fbb.GetSize() }
		};

		co_await socket_.async_send(buffers, ba::use_awaitable);
	}
} catch(std::exception& e) {
	close();
}

void PeerConnection::send(std::unique_ptr<Message> buffer) {
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
ba::awaitable<std::size_t> PeerConnection::read_until(const std::size_t offset,
                                                      const std::size_t read_size) {
	std::size_t received = offset;

	while(received < read_size) {
		auto buffer = ba::buffer(buffer_.data() + received, buffer_.size() - received);
		received += co_await socket_.async_receive(buffer, ba::use_awaitable);
	}

	co_return received;
}

ba::awaitable<std::size_t> PeerConnection::do_receive(const std::size_t offset) {
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

	// message complete, get it handled
	std::span view(buffer_.data(), msg_size);
	dispatcher_.receive(view);

	// move any data belonging to the next message to the start
	if(rcv_size > msg_size) {
		std::memmove(buffer_.data(), buffer_.data() + msg_size, buffer_.size() - msg_size);
	}

	assert(msg_size <= rcv_size);
	co_return rcv_size - msg_size; // offset to start the next read at
}

ba::awaitable<void> PeerConnection::receive() try {
	std::size_t offset = 0;

	while(socket_.is_open()) {
		offset = co_await do_receive(offset);
	}
} catch(std::exception& e) {
	close();
}

std::string PeerConnection::address() {
	if(!socket_.is_open()) {
		return "";
	}

	const auto& ep = socket_.remote_endpoint();
	return std::format("{}:{}", ep.address().to_string(), ep.port());
}

// todo, need to inform the handler when an error occurs
void PeerConnection::close() {
	socket_.close();
}

} // spark, ember