/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/pcp/DatagramTransport.h>
#include <boost/asio/ip/multicast.hpp>

namespace ember::ports {

DatagramTransport::DatagramTransport(const std::string& bind, std::uint16_t port, ba::io_context& ctx)
	: ctx_(ctx), strand_(ctx),
	  socket_(ctx_, ba::ip::udp::endpoint(ba::ip::address::from_string(bind), port)),
	  resolver_(ctx_) {
	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));

	ctx_.post(strand_.wrap([&]() {
		receive();
	}));
}

DatagramTransport::~DatagramTransport() {
	socket_.close();
	ctx_.stop();
}

void DatagramTransport::join_group(const std::string& address) {
	const auto group_ip = boost::asio::ip::address::from_string(address);
	const auto mcast_iface = socket_.local_endpoint().address();
	ba::ip::multicast::join_group join_opt{};

	// ASIO is doing something weird on Windows, this is a hack
	if(mcast_iface.is_v4()) {
		join_opt = ba::ip::multicast::join_group(group_ip.to_v4(), mcast_iface.to_v4());
	}

	socket_.set_option(join_opt);
}

void DatagramTransport::resolve(std::string_view host, const std::uint16_t port, OnResolve&& cb) {
	resolver_.async_resolve(host, std::to_string(port), strand_.wrap(
		[&, cb = std::move(cb)](const boost::system::error_code& ec,
		                        ba::ip::udp::resolver::results_type results) {
			if(!ec) {
				remote_ep_ = results->endpoint();
			}

			cb(ec, remote_ep_);
		}
	));
}

void DatagramTransport::do_write() {
	auto datagram = std::move(queue_.front());
	auto buffer = boost::asio::buffer(datagram->data(), datagram->size());
	queue_.pop();

	socket_.async_send_to(buffer, remote_ep_, strand_.wrap(
		[this, dg = std::move(datagram)](boost::system::error_code ec,
		                                 std::size_t /*bytes_sent*/) {
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
	));
}

void DatagramTransport::send(std::shared_ptr<std::vector<std::uint8_t>> message) {
	ctx_.post(strand_.wrap([&, datagram = std::move(message)]() mutable {
		queue_.emplace(std::move(datagram));

		if(queue_.size() == 1) {
			do_write();
		}
	}));
}

void DatagramTransport::send(std::vector<std::uint8_t> message) {
	auto datagram = std::make_shared<std::vector<std::uint8_t>>(std::move(message));
	send(std::move(datagram));
}

void DatagramTransport::receive() {
	socket_.async_receive_from(boost::asio::null_buffers(), ep_, strand_.wrap(
		[this](boost::system::error_code ec, std::size_t length) {
			if(ec == boost::asio::error::operation_aborted) {
				return;
			} else if(ec) {
				ecb_(ec);
				return;
			}

			std::vector<std::uint8_t> buffer(socket_.available());
			boost::asio::socket_base::message_flags flags(0);
			const std::size_t recv = socket_.receive_from(boost::asio::buffer(buffer), ep_, flags, ec);
			buffer.resize(recv);

			if(!ec) {
				rcb_(buffer, ep_);
			} else {
				ecb_(ec);
			}

			receive();
		}));
}

void DatagramTransport::close() {
	socket_.close();
}

void DatagramTransport::set_callbacks(OnReceive rcb, OnConnectionError ecb) {
	if(!rcb || !ecb) {
		throw std::invalid_argument("Transport callbacks cannot be null");
	}

	rcb_ = rcb;
	ecb_ = ecb;
}

} // ports, ember