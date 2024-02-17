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
	: socket_(ctx_), timeout_(timeout), retries_(retries) { }

DatagramTransport::~DatagramTransport() {
	socket_.close();
}

void DatagramTransport::connect(std::string_view host, const std::uint16_t port) {
	ba::ip::udp::resolver resolver(ctx_);
	auto endpoints = resolver.resolve(host, std::to_string(port));
	ep_ = boost::asio::connect(socket_, endpoints);
}

// todo, proper queue
void DatagramTransport::send(std::vector<std::uint8_t> message) {
	auto datagram = std::make_unique<std::vector<std::uint8_t>>(std::move(message));
	auto buffer = boost::asio::buffer(datagram->data(), datagram->size());

	socket_.async_send_to(buffer, ep_,
		[this, dg = std::move(datagram)](boost::system::error_code ec, std::size_t /*bytes_sent*/) {
			if(!ec) {
				receive();
			}
		});
}

void DatagramTransport::receive() {
	socket_.async_receive_from(boost::asio::null_buffers(), ep_,
		[this](boost::system::error_code ec, std::size_t /*length*/) {
			if(ec == boost::asio::error::operation_aborted) {
				return;
			} else if(ec) {
				ecb_(ec);
				return;
			}


			std::vector<std::uint8_t> buffer(socket_.available());
			boost::asio::socket_base::message_flags flags(0);
			std::size_t recv = socket_.receive_from(boost::asio::buffer(buffer), ep_, flags, ec);
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

boost::asio::io_context* DatagramTransport::executor() {
	return &ctx_;
}

std::string DatagramTransport::local_ip() {
	return socket_.local_endpoint().address().to_string();
}

std::uint16_t DatagramTransport::local_port() {
	return socket_.local_endpoint().port();
}

} // stun, ember