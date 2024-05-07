/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/DatagramTransport.h>

namespace ember::stun {

DatagramTransport::DatagramTransport(const std::string& bind,
                                     std::chrono::milliseconds timeout,
                                     unsigned int retries)
	: socket_(ctx_, ba::ip::udp::endpoint(ba::ip::address::from_string(bind), 0)),
	timeout_(timeout), retries_(retries), resolver_(ctx_) {
	worker_ = std::jthread(static_cast<size_t(boost::asio::io_context::*)()>
		(&boost::asio::io_context::run), &ctx_);

	ctx_.post([&]() {
		receive();
	});
}

DatagramTransport::~DatagramTransport() {
	socket_.close();
	ctx_.stop();
}

void DatagramTransport::connect(std::string_view host, const std::uint16_t port, OnConnect&& cb) {
	resolver_.async_resolve(host, std::to_string(port),
		[&, cb = std::move(cb)](const boost::system::error_code& ec,
		                        ba::ip::udp::resolver::results_type results) {
			if(!ec) {
				remote_ep_ = results->endpoint();
			}

			cb(ec);
		}
	);
}

void DatagramTransport::do_write() {
	auto datagram = std::move(queue_.front());
	auto buffer = boost::asio::buffer(datagram->data(), datagram->size());
	queue_.pop();

	socket_.async_send_to(buffer, remote_ep_,
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
	socket_.async_receive_from(boost::asio::null_buffers(), ep_,
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
				rcb_(std::move(buffer));
			} else {
				ecb_(ec);
			}

			receive();
		});
}

void DatagramTransport::close() {
	socket_.close();
}

std::chrono::milliseconds DatagramTransport::timeout() const {
	return timeout_;
}

unsigned int DatagramTransport::retries() const {
	return retries_;
}

std::string DatagramTransport::local_ip() const {
	const auto& address = socket_.local_endpoint().address();
	auto local_ip = address.to_string();

	/*
     * Hack to try to work around ASIO always reporting the local IP as
	 * 0.0.0.0 when told to listen on every interface - it will not always
	 * work!
	 * 
	 * ASIO can't enumerate NICs, so the only robust solution is to
	 * encourage the user to explicitly bind to an interface (supported)
	 */
	if(local_ip == "0.0.0.0" || local_ip == "::") {
		ba::ip::tcp::resolver resolver(ctx_);
		const ba::ip::tcp::resolver::query query(ba::ip::host_name(), "");

		const auto results = resolver.resolve(query);

		for(const auto& entry : results) {
			if((entry.endpoint().address().is_v4()
				&& address.is_v4())
				|| (entry.endpoint().address().is_v6()
				&& address.is_v6())) {
				local_ip = entry.endpoint().address().to_string();
			}
		}
	}
	
	return local_ip;
}

std::uint16_t DatagramTransport::local_port() const {
	return socket_.local_endpoint().port();
}

} // stun, ember