/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/DatagramTransport.h>

namespace ember::stun {

DatagramTransport::DatagramTransport(std::chrono::milliseconds timeout, unsigned int retries)
	: socket_(ctx_), timeout_(timeout), retries_(retries), resolver_(ctx_) { 
	work_.emplace_back(std::make_shared<boost::asio::io_context::work>(ctx_));
	worker_ = std::jthread(static_cast<size_t(boost::asio::io_context::*)()>
		(&boost::asio::io_context::run), &ctx_);
}

DatagramTransport::~DatagramTransport() {
	socket_.close();
	ctx_.stop();
	work_.clear();
}

void DatagramTransport::connect(std::string_view host, const std::uint16_t port, OnConnect cb) {
	resolver_.async_resolve(host, std::to_string(port),
		[&, cb](const boost::system::error_code& ec, ba::ip::udp::resolver::results_type results) {
			if(!ec) {
				do_connect(std::move(results), cb);
			} else {
				cb(ec);
			}
		}
	);
}

void DatagramTransport::do_connect(ba::ip::udp::resolver::results_type results, OnConnect cb) {
	boost::asio::async_connect(socket_, results.begin(), results.end(),
		[&, cb, results](const boost::system::error_code& ec, ba::ip::udp::resolver::iterator) {
			if(!ec) {
				receive();
			}

			cb(ec);
		}
	);
}

void DatagramTransport::do_write() {
	auto datagram = std::move(queue_.front());
	auto buffer = boost::asio::buffer(datagram->data(), datagram->size());
	queue_.pop();

	socket_.async_send(buffer,
		[this, dg = std::move(datagram)](boost::system::error_code ec, std::size_t /*bytes_sent*/) {
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

void DatagramTransport::send(std::shared_ptr<std::vector<std::uint8_t>> message) {
	ctx_.post([&, datagram = std::move(message)]() mutable {
		queue_.emplace(std::move(datagram));

		if(queue_.size() == 1) {
			do_write();
		}
	});
}

void DatagramTransport::send(std::vector<std::uint8_t> message) {
	auto datagram = std::make_shared<std::vector<std::uint8_t>>(std::move(message));
	send(std::move(datagram));
	
}

void DatagramTransport::receive() {
	socket_.async_receive(boost::asio::null_buffers(),
		[this](boost::system::error_code ec, std::size_t length) {
			if(ec == boost::asio::error::operation_aborted) {
				return;
			} else if(ec) {
				ecb_(ec);
				return;
			}

			std::vector<std::uint8_t> buffer(socket_.available());
			boost::asio::socket_base::message_flags flags(0);
			const std::size_t recv = socket_.receive(boost::asio::buffer(buffer), flags, ec);
			buffer.resize(recv);

			if(!ec) {
				rcb_(std::move(buffer));
			} else {
				ecb_(ec);
			}
		});
}

void DatagramTransport::close() {
	socket_.close();
}

std::chrono::milliseconds DatagramTransport::timeout() {
	return timeout_;
}

unsigned int DatagramTransport::retries() {
	return retries_;
}

std::string DatagramTransport::local_ip() {
	return socket_.local_endpoint().address().to_string();
}

std::uint16_t DatagramTransport::local_port() {
	return socket_.local_endpoint().port();
}

} // stun, ember