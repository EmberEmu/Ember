/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/StreamTransport.h>
#include <stun/Protocol.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/VectorBufferAdaptor.h>

namespace ember::stun {

StreamTransport::StreamTransport(std::string_view host, std::uint16_t port,
	std::chrono::milliseconds timeout)
	: host_(host), port_(port), timeout_(timeout), socket_(ctx_) { }

StreamTransport::~StreamTransport() {
	socket_.close();
}

void StreamTransport::connect() {
	ba::ip::tcp::resolver resolver(ctx_);
	auto endpoints = resolver.resolve(host_, std::to_string(port_));
	boost::asio::connect(socket_, endpoints); // not async, todo, error handle
	receive();
}

void StreamTransport::send(std::vector<std::uint8_t> message) {
	auto data = std::make_unique<std::vector<std::uint8_t>>(std::move(message));
	auto buffer = boost::asio::buffer(data->data(), data->size());

	ba::async_write(socket_, buffer,
		[this, d = std::move(data)](boost::system::error_code ec, std::size_t /*sent*/) {
			if (!ec) {
				// todo
			}
		});
}

void StreamTransport::receive() {
	switch (state_) {
		case ReadState::READ_BODY:
			state_ = ReadState::READ_DONE;
			read(get_length(), HEADER_LENGTH);
			break;
		case ReadState::READ_DONE:
			state_ = ReadState::READ_HEADER;
			rcb_(std::move(buffer_));
			[[fallthrough]];
		case ReadState::READ_HEADER:
			state_ = ReadState::READ_BODY;
			read(HEADER_LENGTH, 0);
			break;
	}
}

std::size_t StreamTransport::get_length() {
	spark::VectorBufferAdaptor<std::uint8_t> vba(buffer_);
	spark::BinaryStream stream(vba);

	Header header{};
	stream >> header.type;
	stream >> header.length;
	return header.length;
}

void StreamTransport::read(const std::size_t size, const std::size_t offset) {
	buffer_.resize(buffer_.size() + size); // todo, cap size
	const auto buffer = boost::asio::buffer(buffer_.data() + offset, size);

	boost::asio::async_read(socket_, buffer,
		[this](boost::system::error_code ec, std::size_t size) {
			if(ec == boost::asio::error::operation_aborted) {
				return;
			}

			if(!ec) {
				receive();
			} else {
				ecb_(ec);
			}
		}
	);
}

void StreamTransport::close() {

}

std::chrono::milliseconds StreamTransport::timeout() {
	return timeout_;
}

unsigned int StreamTransport::retries() {
	return 0;
}

boost::asio::io_context* StreamTransport::executor() {
	return &ctx_;
}

} // stun, ember