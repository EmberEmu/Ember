/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ember::ports {

class MulticastSocket final {
	using OnReceive = std::function<void(std::span<const std::uint8_t>,
	                                     const boost::asio::ip::udp::endpoint&)>;

    std::array<std::uint8_t, 4096> buffer_;

    boost::asio::io_context& context_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint ep_, remote_ep_;
	
	OnReceive rcv_handler_{};

    void receive();
    void handle_datagram(std::span<const std::uint8_t> datagram,
	                     const boost::asio::ip::udp::endpoint& ep);

public:
    MulticastSocket(boost::asio::io_context& context,
                    const std::string& listen_addr,
                    const std::string& mcast_group,
                    std::uint16_t port);

	void send(std::vector<std::uint8_t> buffer);
    void send(std::shared_ptr<std::vector<std::uint8_t>> buffer);
    void set_callbacks(OnReceive&& rcv);
	void close();
};

} // port, ember