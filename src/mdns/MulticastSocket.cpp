/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "MulticastSocket.h"

namespace ember::dns {

MulticastSocket::MulticastSocket(boost::asio::io_context& context,
                                 const std::string& listen_addr,
                                 const std::string& mcast_group,
                                 const std::uint16_t port)
                                 : context_(context),
                                   socket_(context),
                                   ep_(boost::asio::ip::address::from_string(mcast_group), port) {
    const auto listen_ip = boost::asio::ip::address::from_string(listen_addr);
    const auto group_ip = boost::asio::ip::address::from_string(mcast_group);

	boost::asio::ip::udp::endpoint ep(listen_ip, port);
	socket_.open(ep.protocol());
	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	socket_.set_option(boost::asio::ip::multicast::join_group(group_ip));
	socket_.bind(ep);
    receive();
}

void MulticastSocket::receive() {
	socket_.async_receive_from(boost::asio::buffer(buffer_.data(), buffer_.size()), remote_ep_,
        [this](const boost::system::error_code& ec, const std::size_t size) {
            if(ec && ec == boost::asio::error::operation_aborted) {
		        return;
	        }

	        if(!ec) {
		        handle_datagram({buffer_.data(), size});
	        }

	        receive();
        }
    );
}

void MulticastSocket::handle_datagram(std::span<std::byte> buffer) {
    if(!handler_) {
        // log
        return;
    }

    handler_->handle_datagram(buffer);
}

void MulticastSocket::send() {

}

void MulticastSocket::register_handler(Handler* handler) {
    handler_ = handler;
}

} // dns, ember