/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/StreamTransport.h>
#include <stun/Protocol.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/BufferAdaptor.h>
#include <thread/Utility.h>
#include <boost/asio/connect.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

namespace ember::stun {

StreamTransport::StreamTransport(ba::io_context& ctx, std::string_view bind, std::chrono::milliseconds timeout)
	: ctx_(ctx),
	  timeout_(timeout), 
	  socket_(ctx_, ba::ip::tcp::endpoint(ba::ip::make_address(bind), 0)),
	  resolver_(ctx_), 
	  work_(boost::asio::make_work_guard(ctx_)) {
	socket_.set_option(boost::asio::ip::tcp::no_delay(true));
}

StreamTransport::~StreamTransport() {
	socket_.close();
	ctx_.stop();
	work_.reset();
}

void StreamTransport::connect(const std::string_view host, const std::uint16_t port, OnConnect&& cb) {
	resolver_.async_resolve(host, std::to_string(port),
		[&, cb = std::move(cb)](const boost::system::error_code& ec,
		         ba::ip::tcp::resolver::results_type endpoints) mutable {
			if(!ec) {
				do_connect(std::move(endpoints), std::move(cb));
			} else {
				cb(ec);
			}
		}
	);
}

void StreamTransport::do_connect(ba::ip::tcp::resolver::results_type endpoints, OnConnect&& cb) {
	boost::asio::async_connect(socket_, endpoints,
		[&, cb = std::move(cb)](const boost::system::error_code& ec, ba::ip::tcp::endpoint) {
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
	queue_.pop();

	ba::async_write(socket_, boost::asio::buffer(*data),
		[this, d = data](boost::system::error_code ec, std::size_t /*sent*/) {
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
	boost::asio::post(ctx_, [&, message]() mutable {
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
			rcb_(buffer_);
			buffer_.clear();
			[[fallthrough]];
		case ReadState::READ_HEADER:
			state_ = ReadState::READ_BODY;
			read(HEADER_LENGTH, 0);
			break;
	}
}

std::size_t StreamTransport::get_length() {
	spark::io::BufferAdaptor vba(buffer_);
	spark::io::BinaryStream stream(vba);

	Header header{};
	stream >> header.type;
	stream >> header.length;
	return header.length;
}

void StreamTransport::read(const std::size_t size, const std::size_t offset) {
	buffer_.resize(buffer_.size() + size); // todo, cap size
	const auto buffer = boost::asio::buffer(buffer_.data() + offset, size);

	boost::asio::async_read(socket_, buffer,
		[this](const boost::system::error_code& ec, std::size_t) {
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