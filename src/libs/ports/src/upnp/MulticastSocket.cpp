/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/MulticastSocket.h>
#include <utility>

namespace ember::ports {

MulticastSocket::MulticastSocket(boost::asio::io_context& context, const std::string& listen_iface,
                                 const std::string& mcast_group, const std::uint16_t port)
	: context_(context), socket_(context), ep_(boost::asio::ip::address::from_string(mcast_group), port) {
    const auto mcast_iface = boost::asio::ip::address::from_string(listen_iface);
    const auto group_ip = boost::asio::ip::address::from_string(mcast_group);

	boost::asio::ip::udp::endpoint ep(mcast_iface, 0);
	socket_.open(ep.protocol());
	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	boost::asio::ip::multicast::join_group join_opt{};

	// ASIO is doing something weird on Windows, this is a hack
	if(mcast_iface.is_v4()) {
		join_opt = boost::asio::ip::multicast::join_group(group_ip.to_v4(), mcast_iface.to_v4());
	} else {
		join_opt = boost::asio::ip::multicast::join_group(group_ip);
	}

	socket_.set_option(join_opt);
	socket_.bind(ep);
    receive();
}

void MulticastSocket::receive() {
	if(!socket_.is_open()) {
		return;
	}

	socket_.async_receive_from(boost::asio::buffer(buffer_.data(), buffer_.size()), remote_ep_,
        [this](const boost::system::error_code& ec, const std::size_t size) {
            if(ec && ec == boost::asio::error::operation_aborted) {
		        return;
	        }

	        if(!ec) {
		        handle_datagram({buffer_.data(), size}, remote_ep_);
	        }

	        receive();
        }
    );
}

void MulticastSocket::handle_datagram(std::span<const std::uint8_t> datagram,
                                      const boost::asio::ip::udp::endpoint& ep) {
	rcv_handler_(datagram, ep);
}

void MulticastSocket::send(std::vector<std::uint8_t> buffer, ba::ip::udp::endpoint ep) {
	auto ptr = std::make_shared<decltype(buffer)>(std::move(buffer));
	send(std::move(ptr), ep);
}

void MulticastSocket::send(std::vector<std::uint8_t> buffer) {
	auto ptr = std::make_shared<decltype(buffer)>(std::move(buffer));
	send(std::move(ptr));
}

void MulticastSocket::send(std::shared_ptr<std::vector<std::uint8_t>> buffer,
                           ba::ip::udp::endpoint ep) {
	if(!socket_.is_open()) {
		return;
	}

	const auto ba_buf = boost::asio::buffer(*buffer);

	socket_.async_send_to(ba_buf, ep,
		[&, buff = std::move(buffer)](const boost::system::error_code& ec, std::size_t size) {
			if(ec) {
				socket_.close();
			}
		}
	);
}

void MulticastSocket::send(std::shared_ptr<std::vector<std::uint8_t>> buffer) {
	send(std::move(buffer), ep_);
}

void MulticastSocket::set_callbacks(OnReceive&& rcv) {
	rcv_handler_ = rcv;
}

void MulticastSocket::close() {
	boost::system::error_code ec; // we don't care about any errors
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	socket_.close(ec);
}

std::string MulticastSocket::local_address() const {
	return socket_.local_endpoint().address().to_string();
}

} // ports, ember