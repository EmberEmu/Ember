/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/DatagramTransport.h>
#include <iostream>
namespace ember::stun {

DatagramTransport::DatagramTransport(ba::io_context& ctx, const std::string& host,
                                     std::uint16_t port, ReceiveCallback rcb)
	: ctx_(ctx), host_(host), port_(port), socket_(ctx, ba::ip::udp::endpoint(ba::ip::udp::v4(), 0)), rcb_(rcb) { }

DatagramTransport::~DatagramTransport() {
	socket_.close();
}

void DatagramTransport::connect() {
	ba::ip::udp::resolver resolver(ctx_);
	ba::ip::udp::resolver::query query(host_, std::to_string(port_));
	ep_ = boost::asio::connect(socket_, resolver.resolve(query));
}

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
			if(ec) {
				return;
			}

			std::vector<std::uint8_t> buffer(socket_.available());
			boost::asio::socket_base::message_flags flags(0);
			std::size_t recv = socket_.receive_from(boost::asio::buffer(buffer), ep_, flags, ec);
			buffer.resize(recv);

			if(!ec) {
				rcb_(std::move(buffer));
			}
		});
}

} // stun, ember