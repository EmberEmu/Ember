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
#include <spark/buffers/BufferAdaptor.h>

namespace ember::stun {

StreamTransport::StreamTransport(const std::string& bind, std::chrono::milliseconds timeout)
	: timeout_(timeout), 
	  socket_(ctx_, ba::ip::tcp::endpoint(ba::ip::address::from_string(bind), 0)),
	  resolver_(ctx_) {
	work_ = std::make_unique<boost::asio::io_context::work>(ctx_);
	worker_ = std::jthread(static_cast<size_t(boost::asio::io_context::*)()>
		(&boost::asio::io_context::run), &ctx_);
}

StreamTransport::~StreamTransport() {
	socket_.close();
	ctx_.stop();
	work_.reset();
}

void StreamTransport::connect(std::string_view host, const std::uint16_t port, OnConnect&& cb) {
	resolver_.async_resolve(host, std::to_string(port),
		[&, cb = std::move(cb)](const boost::system::error_code& ec,
		        ba::ip::tcp::resolver::results_type results) mutable {
			if(!ec) {
				do_connect(std::move(results), std::move(cb));
			} else {
				cb(ec);
			}
		}
	);
}

void StreamTransport::do_connect(ba::ip::tcp::resolver::results_type results, OnConnect&& cb) {
	boost::asio::async_connect(socket_, results.begin(), results.end(),
		[&, cb = std::move(cb), results](const boost::system::error_code& ec,
		                 ba::ip::tcp::resolver::iterator) {
			if(!ec) {
				state_ = ReadState::READ_HEADER;
				receive();
			}

			cb(ec);
		}
	);
}

void StreamTransport::do_write() {
	auto data = std::move(queue_.front());
	auto buffer = boost::asio::buffer(data->data(), data->size());
	queue_.pop();

	ba::async_write(socket_, buffer,
		[this, d = std::move(data)](boost::system::error_code ec, std::size_t /*sent*/) {
			if(ec == boost::asio::error::operation_aborted) {
				return;
			} else if(ec) {
				ecb_(ec);
				return;
			}

			if(!queue_.empty()) {
				do_write();
			}
		}
	);
}

void StreamTransport::send(std::shared_ptr<std::vector<std::uint8_t>> message) {
	ctx_.post([&, message]() mutable {
		queue_.emplace(std::move(message));

		if(queue_.size() == 1) {
			do_write();
		}
	});
}

void StreamTransport::send(std::vector<std::uint8_t> message) {
	auto data = std::make_shared<std::vector<std::uint8_t>>(std::move(message));
	send(std::move(data));
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
	spark::BufferAdaptor vba(buffer_);
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
	socket_.close();
}

std::chrono::milliseconds StreamTransport::timeout() const {
	return timeout_;
}

unsigned int StreamTransport::retries() const {
	return 0;
}

std::string StreamTransport::local_ip() const {
	return socket_.local_endpoint().address().to_string();
}

std::uint16_t StreamTransport::local_port() const {
	return socket_.local_endpoint().port();
}

} // stun, ember