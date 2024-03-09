/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio.hpp>
#include <array>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>
#include <cstdint>

namespace ember::ports {

namespace ba = boost::asio;

class MulticastSocket final {
    std::array<std::uint8_t, 4096> buffer_;

    ba::io_context& context_;
    ba::ip::udp::socket socket_;
    ba::ip::udp::endpoint ep_, remote_ep_;

public:
	using ReceiveType = std::expected<std::span<std::uint8_t>, boost::system::error_code>;

    MulticastSocket(boost::asio::io_context& context,
                    const std::string& listen_addr,
                    const std::string& mcast_group,
                    std::uint16_t port);
	~MulticastSocket();

	ba::awaitable<bool> send(std::vector<std::uint8_t> buffer, ba::ip::udp::endpoint);
	ba::awaitable<bool> send(std::vector<std::uint8_t> buffer);
	ba::awaitable<bool> send(std::shared_ptr<std::vector<std::uint8_t>> buffer);
	ba::awaitable<bool> send(std::shared_ptr<std::vector<std::uint8_t>> buffer, ba::ip::udp::endpoint);
	ba::awaitable<ReceiveType> receive();
	std::string local_address() const;
	void close();
};

} // port, ember