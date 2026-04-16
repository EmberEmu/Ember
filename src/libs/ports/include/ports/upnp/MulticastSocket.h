/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <array>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <cstdint>

namespace ember::ports {

class MulticastSocket final {
    std::array<std::uint8_t, 4096> buffer_;

    boost::asio::io_context& context_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint ep_, remote_ep_;

public:
	using Result = std::expected<std::span<std::uint8_t>, boost::system::error_code>;

    MulticastSocket(boost::asio::io_context& context,
                    std::string_view listen_addr,
                    std::string_view mcast_group,
                    std::uint16_t port);
	~MulticastSocket();

	boost::asio::awaitable<bool> send(std::span<const std::uint8_t> buffer, boost::asio::ip::udp::endpoint);
	boost::asio::awaitable<bool> send(std::span<const std::uint8_t> buffer);
	boost::asio::awaitable<Result> receive();
	std::string local_address() const;
	void close();
};

} // port, ember